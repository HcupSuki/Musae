// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

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

        if(!terrain.CsvLoad())    
        {
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
        }
        else
        {
            std::vector<std::vector<G4double>> data;
            const auto nx{terrain.CsvNx()},ny{terrain.CsvNy()};
            std::ifstream file(terrain.ElevationDataPath().c_str());
            if (!file.is_open()) {
                std::cerr << "Error opening file!" << std::endl;
                throw std::runtime_error("File not found.");
            }
            // std::cout << "File opened successfully." << std::endl;
            // std::cout << "NX = " << nx << std::endl;
            // std::cout << "NY = " << ny << std::endl;

            std::string header;
            std::getline(file, header);


            std::string line;
            while (std::getline(file, line)) {
                std::vector<G4double> row;
                std::stringstream ss(line);
                std::string cell;

                while (std::getline(ss, cell, ',')) {
                    row.push_back(stod(cell));
                }

                data.push_back(row);
            }


            // add vertices
            Eigen::MatrixX<pmp::Vertex> t(nx, ny);    // terrain
            std::map<std::pair<int, int>, pmp::Vertex> b; // bottom corner
            for (int j{}; j < ny; ++j) {
                for (int i{}; i < nx; ++i) {
                    t(i, j) = mesh->add_vertex(pmp::Point{std::move(data[nx*j+i][0]*1000), std::move(data[nx*j+i][1]*1000), std::move(data[nx*j+i][2]*1000)});
                    if ((i == 0 and j == 0) or
                        (i == 0 and j == ny-1) or
                        (i == nx-1 and j == 0) or
                        (i == nx-1 and j == ny-1)) {
                        b[{i, j}] = {mesh->add_vertex(pmp::Point{std::move(data[nx*j+i][0]*1000), std::move(data[nx*j+i][1]*1000), terrain.MinZ()})};
                    }
                }
            }

            // add terrain faces
            for (int i{}; i < nx-1; ++i) {
                for (int j{}; j < ny-1; ++j) {
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
            mesh->add_quad(b.at({0, 0}), b.at({nx-1, 0}), b.at({nx-1, ny-1}), b.at({0, ny-1}));

            // add front faces
            std::vector<pmp::Vertex> edge;
            for (int j{ny-1}; j >= 0; --j) {
                edge.emplace_back(t(0, j));
            }
            edge.emplace_back(b.at({0, 0}));
            edge.emplace_back(b.at({0, ny-1}));
            mesh->add_face(edge);

            // add front faces
            edge.clear();
            for (int i{}; i < nx; ++i) {
                edge.emplace_back(t(i, 0));
            }
            edge.emplace_back(b.at({nx-1, 0}));
            edge.emplace_back(b.at({0, 0}));
            mesh->add_face(edge);

            // add front faces
            edge.clear();
            for (int j{}; j < ny; ++j) {
                edge.emplace_back(t(nx-1, j));
            }
            edge.emplace_back(b.at({nx-1, ny-1}));
            edge.emplace_back(b.at({nx-1, 0}));
            mesh->add_face(edge);

            // add front faces
            edge.clear();
            for (int i{nx-1}; i >= 0; --i) {
                edge.emplace_back(t(i, ny-1));
            }
            edge.emplace_back(b.at({0, ny-1}));
            edge.emplace_back(b.at({nx-1, ny-1}));
            mesh->add_face(edge);
            // std::cout << "File add faces." << std::endl;
            // triangulate faces
            pmp::triangulate(*mesh);
            // std::cout << "File triangulate faces." << std::endl;
            return mesh;
        }
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
    // if(terrain.FractionMass()){
    //     G4double z, a, density;
    //     density = 2.65*CLHEP::g/CLHEP::cm3;
    //     a = 2*CLHEP::g/CLHEP::mole;
    //     G4Material* StandardRock = new G4Material("StandardRock", z=11, a, density);
    //     const auto logic{Make<G4LogicalVolume>(
    //         solid,
    //         StandardRock,
    //         terrain.Name())};
    //     Make<G4PVPlacement>(
    //         G4Transform3D{},
    //         logic,
    //         terrain.Name(),
    //         Mother().LogicalVolume(),
    //         false,
    //         0,
    //         checkOverlaps);
    // }
    G4Material* material = G4NistManager::Instance()->FindOrBuildMaterial(terrain.Material().c_str());
        if (!material) {
            material = G4Material::GetMaterial(terrain.Material().c_str());
            if (!material) {
                throw std::runtime_error{"ERROR: Material '"};
            }
        }
    const auto logic{Make<G4LogicalVolume>(
        solid,
        material,
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
