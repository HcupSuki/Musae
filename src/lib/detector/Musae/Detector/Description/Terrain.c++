#include "Musae/Detector/Description/Terrain.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/VectorArithmeticOperator.h++"
#include "Mustard/Utility/VectorCast.h++"

#include "turtle.h"

#include "G4SystemOfUnits.hh"

#include <climits>
#include <cmath>

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Density;
using namespace Mustard::LiteralUnit::Length;
using namespace Mustard::VectorArithmeticOperator::Vector3ArithmeticOperator;

Terrain::Terrain() : // clang-format off
    DescriptionWithCacheBase{"Terrain"}, // clang-format on
    fElevationDataPath{this, "terrain"},
    fProjection{this, "UTM 50N"},
    fReferenceLatitude{this, 25.11},
    fReferenceLongitude{this, 113.65},
    fReferenceElevation{this, std::numeric_limits<double>::quiet_NaN()},
    fMinLatitude{this, fReferenceLatitude - 0.01},
    fMaxLatitude{this, fReferenceLatitude + 0.01},
    fMinLongitude{this, fReferenceLongitude - 0.01},
    fMaxLongitude{this, fReferenceLongitude + 0.01},
    fMinZ{this, -230_m},
    fNVertex{this, 10000},
    fRockElement{this, {"Ca", "Fe", "Al", "Si", "O"}},
    fRockNAtom{this, {3, 1, 1, 3, 12}},
    fRockDensity{this, 3.0_g_cm3},
    fTurtleStack{this, [this] { return CreateTurtleStack(); }},
    fTurtleProjection{this, [this] { return CreateTurtleProjection(); }},
    fReferenceXYZ{this, [this] { return ProjectReferenceXYZ(); }} {}

auto Terrain::Elevation(double latitude, double longitude) const -> double {
    double elevation;
    turtle_stack_elevation(fTurtleStack->get(), latitude, longitude, &elevation, nullptr);
    return elevation;
}

auto Terrain::Project(double latitude, double longitude, double elevation) const -> muc::array3d {
    muc::array3d xyz;
    turtle_projection_project(fTurtleProjection->get(), latitude, longitude, &xyz[0], &xyz[1]);
    xyz[2] = elevation;
    xyz *= m;
    xyz -= *fReferenceXYZ;
    return xyz;
}

auto Terrain::Unproject(muc::array3d xyz) const -> std::tuple<double, double, double> {
    std::tuple<double, double, double> lle;
    xyz += *fReferenceXYZ;
    xyz /= m;
    turtle_projection_unproject(fTurtleProjection->get(), xyz[0], xyz[1], &get<0>(lle), &get<1>(lle));
    get<2>(lle) = xyz[2];
    return lle;
}

auto Terrain::TurtleDeleter::operator()(turtle_stack* p) -> void {
    turtle_stack_destroy(&p);
}

auto Terrain::TurtleDeleter::operator()(turtle_projection* p) -> void {
    turtle_projection_destroy(&p);
}

auto Terrain::CreateTurtleStack() const -> std::unique_ptr<turtle_stack, TurtleDeleter> {
    turtle_stack* stack;
    turtle_stack_create(&stack, fElevationDataPath->c_str(), 1000000, nullptr, nullptr);
    return std::unique_ptr<turtle_stack, TurtleDeleter>{stack};
}

auto Terrain::CreateTurtleProjection() const -> std::unique_ptr<turtle_projection, TurtleDeleter> {
    turtle_projection* projection;
    turtle_projection_create(&projection, fProjection->c_str());
    return std::unique_ptr<turtle_projection, TurtleDeleter>{projection};
}

auto Terrain::ProjectReferenceXYZ() const -> muc::array3d {
    muc::array3d xyz;
    turtle_projection_project(fTurtleProjection->get(), fReferenceLatitude, fReferenceLongitude, &xyz[0], &xyz[1]);
    if (std::isnan(fReferenceElevation)) {
        turtle_stack_elevation(fTurtleStack->get(), fReferenceLatitude, fReferenceLongitude, &xyz[2], nullptr);
        Mustard::PrintInfo(std::to_string(xyz[2]));
    } else {
        xyz[2] = fReferenceElevation;
    }
    return xyz * m;
}

auto Terrain::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fElevationDataPath, "ElevationDataPath");
    ImportValue(node, fProjection, "Projection");
    ImportValue(node, fReferenceLatitude, "ReferenceLatitude");
    ImportValue(node, fReferenceLongitude, "ReferenceLongitude");
    ImportValue(node, fReferenceElevation, "ReferenceElevation");
    ImportValue(node, fMinLatitude, "MinLatitude");
    ImportValue(node, fMaxLatitude, "MaxLatitude");
    ImportValue(node, fMinLongitude, "MinLongitude");
    ImportValue(node, fMaxLongitude, "MaxLongitude");
    ImportValue(node, fMinZ, "MinZ");
    ImportValue(node, fNVertex, "NVertex");
    ImportValue(node, fRockElement, "RockElement");
    ImportValue(node, fRockNAtom, "RockNAtom");
    ImportValue(node, fRockDensity, "RockDensity");
}

auto Terrain::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fElevationDataPath, "ElevationDataPath");
    ExportValue(node, fProjection, "Projection");
    ExportValue(node, fReferenceLatitude, "ReferenceLatitude");
    ExportValue(node, fReferenceLongitude, "ReferenceLongitude");
    ExportValue(node, fReferenceElevation, "ReferenceElevation");
    ExportValue(node, fMinLatitude, "MinLatitude");
    ExportValue(node, fMaxLatitude, "MaxLatitude");
    ExportValue(node, fMinLongitude, "MinLongitude");
    ExportValue(node, fMaxLongitude, "MaxLongitude");
    ExportValue(node, fMinZ, "MinZ");
    ExportValue(node, fNVertex, "NVertex");
    ExportValue(node, fRockElement, "RockElement");
    ExportValue(node, fRockNAtom, "RockNAtom");
    ExportValue(node, fRockDensity, "RockDensity");
}

} // namespace Musae::Detector::Description
