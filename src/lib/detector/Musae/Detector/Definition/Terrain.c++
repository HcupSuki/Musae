#include "Musae/Detector/Definition/Terrain.h++"
#include "Musae/Detector/Description/Terrain.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/MathConstant.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/VectorCast.h++"

#include "G4LogicalVolume.hh"
#include "G4Material.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Point3D.hh"
#include "G4TessellatedSolid.hh"
#include "G4Transform3D.hh"
#include "G4TriangularFacet.hh"
#include "G4Voxelizer.hh"

#include "pmp/algorithms/triangulation.h"
#include "pmp/surface_mesh.h"

#include "Eigen/Core"

#include "muc/array"
#include "muc/math"

#include "gsl/gsl"

#include <cmath>
#include <concepts>
#include <map>
#include <memory>
#include <utility>
#include <vector>

static_assert(std::same_as<pmp::Scalar, double>, "PMP should be compiled with PMP_SCALAR_TYPE_64");

namespace Musae::Detector::Definition {

using namespace Mustard::MathConstant;
using namespace Mustard::LiteralUnit::Length;

auto Terrain::Construct(bool checkOverlaps) -> void {
    const auto& terrain{Description::Terrain::Instance()};

    // decimated pmp mesh
    const auto mesh{[&] {
        const auto deltaLat{terrain.MaxLatitude() - terrain.MinLatitude()};
        const auto deltaLon{terrain.MaxLongitude() - terrain.MinLongitude()};
        const auto nLat{static_cast<int>(std::llround(std::sqrt((2 * deltaLat) / (sqrt3 * deltaLon) * terrain.NVertex())))};
        const auto nLon{static_cast<int>(std::llround(std::sqrt((sqrt3 * deltaLon) / (2 * deltaLat) * terrain.NVertex())))};
        const auto dLat{deltaLat / (nLat - 1)};
        const auto dLon{deltaLon / (nLon - 1.5)};
        const auto iTail{nLat - 1};
        const auto jTail{nLon - 1};

        auto mesh{std::make_unique_for_overwrite<pmp::SurfaceMesh>()};
        mesh->add_face_property<pmp::Normal>("f:normal");
        mesh->add_face_property<pmp::Vertex>("f:vertex");

        // add vertices
        Eigen::MatrixX<pmp::Vertex> t(nLat, nLon);    // terrain
        std::map<std::pair<int, int>, pmp::Vertex> b; // bottom corner
        for (int i{}; i < nLat; ++i) {
            for (int j{}; j < nLon; ++j) {
                const auto lat{terrain.MinLatitude() + i * dLat};
                auto lon{terrain.MinLongitude() + j * dLon};
                if ((muc::even(i) and j == jTail) or
                    (muc::odd(i) and j != 0)) {
                    lon -= dLon / 2;
                }
                const auto [x, y, z]{terrain.Project(lat, lon, terrain.Elevation(lat, lon))};
                t(i, j) = mesh->add_vertex(pmp::Point{x, y, z});
                if ((i == 0 and j == 0) or
                    (i == 0 and j == jTail) or
                    (i == iTail and j == 0) or
                    (i == iTail and j == jTail)) {
                    b[{i, j}] = {mesh->add_vertex(pmp::Point{x, y, terrain.MinZ()})};
                }
            }
        }

        // add terrain faces
        for (int i{}; i < iTail; ++i) {
            for (int j{}; j < jTail; ++j) {
                if (muc::even(i)) {
                    mesh->add_triangle(t(i, j), t(i + 1, j + 1), t(i + 1, j));
                    mesh->add_triangle(t(i, j), t(i, j + 1), t(i + 1, j + 1));
                } else {
                    mesh->add_triangle(t(i, j + 1), t(i + 1, j), t(i, j));
                    mesh->add_triangle(t(i, j + 1), t(i + 1, j + 1), t(i + 1, j));
                }
            }
        }

        // add bottom face
        mesh->add_quad(b.at({0, 0}), b.at({iTail, 0}), b.at({iTail, jTail}), b.at({0, jTail}));

        // add front faces
        std::vector<pmp::Vertex> edge;
        for (int j{jTail}; j >= 0; --j) {
            edge.emplace_back(t(0, j));
        }
        edge.emplace_back(b.at({0, 0}));
        edge.emplace_back(b.at({0, jTail}));
        mesh->add_face(edge);

        // add front faces
        edge.clear();
        for (int i{}; i < nLat; ++i) {
            edge.emplace_back(t(i, 0));
        }
        edge.emplace_back(b.at({iTail, 0}));
        edge.emplace_back(b.at({0, 0}));
        mesh->add_face(edge);

        // add front faces
        edge.clear();
        for (int j{}; j < nLon; ++j) {
            edge.emplace_back(t(iTail, j));
        }
        edge.emplace_back(b.at({iTail, jTail}));
        edge.emplace_back(b.at({iTail, 0}));
        mesh->add_face(edge);

        // add front faces
        edge.clear();
        for (int i{iTail}; i >= 0; --i) {
            edge.emplace_back(t(i, jTail));
        }
        edge.emplace_back(b.at({0, jTail}));
        edge.emplace_back(b.at({iTail, jTail}));
        mesh->add_face(edge);

        // triangulate faces
        pmp::triangulate(*mesh);

        return mesh;
    }()};

    const auto solid{Make<G4TessellatedSolid>(terrain.Name())};
    for (auto f : mesh->faces()) {
        std::vector<G4ThreeVector> x;
        x.reserve(3);
        for (auto&& v : mesh->vertices(f)) {
            x.emplace_back(Mustard::VectorCast<G4ThreeVector>(mesh->position(v)));
        }
        solid->AddFacet(new G4TriangularFacet{x[0], x[1], x[2], G4FacetVertexType::ABSOLUTE});
    }
    solid->SetSolidClosed(true);
    Mustard::PrintInfo(fmt::format("Number of facets: {}", solid->GetNumberOfFacets()));
    Mustard::PrintInfo(fmt::format("Number of voxels: {}", solid->GetVoxels().GetCountOfVoxels()));

    const auto nist{G4NistManager::Instance()}; // clang-format off
    const auto rock{new G4Material{"Rock", terrain.RockDensity(), static_cast<int>(terrain.RockElement().size()), kStateSolid}}; // clang-format on
    for (gsl::index i{}; i < ssize(terrain.RockElement()); ++i) {
        rock->AddElement(nist->FindOrBuildElement(terrain.RockElement().at(i)), terrain.RockNAtom().at(i));
    }

    const auto logic{Make<G4LogicalVolume>(
        solid,
        rock,
        terrain.Name())};
    Make<G4PVPlacement>(
        G4Transform3D{},
        logic,
        terrain.Name(),
        Mother().LogicalVolume(),
        false,
        0,
        checkOverlaps);
}

} // namespace Musae::Detector::Definition
