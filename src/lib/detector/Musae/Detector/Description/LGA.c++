#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Utility/LiteralUnit.h++"

#include "CLHEP/Vector/EulerAngles.h"
#include "CLHEP/Vector/Rotation.h"

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Energy;
using namespace Mustard::LiteralUnit::Length;
using namespace Mustard::LiteralUnit::Time;

LGA::LGA() :
    DescriptionBase{"LGA"},
    // Geometry
    fPosition{0, 0, 0},
    fEulerAngleAlpha{0.},
    fEulerAngleBeta{0.},
    fEulerAngleGamma{0.},
    fNModule{3},
    fModuleSpacing{15_cm},
    fScintillatorWidth{30_cm},
    fScintillatorThickness{1_cm},
    fUseFastLGA{true},
    fLGACellWidth{1_cm},
    fNLGACellXY{30},
    fLGAThickness{1_cm},
    // Detection
    fEnergyDepositionThreshold{10_keV},
    fTimeResolutionFWHM{1_ns} {}

auto LGA::Transform(int id, double zShift) const -> HepGeom::Transform3D { // clang-format off
    const HepGeom::Rotate3D rotation{CLHEP::HepRotation{fEulerAngleAlpha, fEulerAngleBeta, fEulerAngleGamma}}; // clang-format on
    const auto zLocal{fPosition[2] + id * fModuleSpacing + zShift};
    const HepGeom::Translate3D translation{fPosition[0], fPosition[1], zLocal};
    return translation * rotation;
}

auto LGA::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fPosition, "Position");
    ImportValue(node, fEulerAngleAlpha, "EulerAngleAlpha");
    ImportValue(node, fEulerAngleBeta, "EulerAngleBeta");
    ImportValue(node, fEulerAngleGamma, "EulerAngleGamma");
    ImportValue(node, fNModule, "NModule");
    ImportValue(node, fModuleSpacing, "ModuleSpacing");
    ImportValue(node, fScintillatorWidth, "ScintillatorWidth");
    ImportValue(node, fScintillatorThickness, "ScintillatorThickness");
    ImportValue(node, fUseFastLGA, "UseFastLGA");
    ImportValue(node, fLGACellWidth, "LGACellWidth");
    ImportValue(node, fNLGACellXY, "NLGACellXY");
    ImportValue(node, fLGAThickness, "LGAThickness");
    ImportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ImportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
}

auto LGA::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fPosition, "Position");
    ExportValue(node, fEulerAngleAlpha, "EulerAngleAlpha");
    ExportValue(node, fEulerAngleBeta, "EulerAngleBeta");
    ExportValue(node, fEulerAngleGamma, "EulerAngleGamma");
    ExportValue(node, fNModule, "NModule");
    ExportValue(node, fModuleSpacing, "ModuleSpacing");
    ExportValue(node, fScintillatorWidth, "ScintillatorWidth");
    ExportValue(node, fScintillatorThickness, "ScintillatorThickness");
    ExportValue(node, fUseFastLGA, "UseFastLGA");
    ExportValue(node, fLGACellWidth, "LGACellWidth");
    ExportValue(node, fNLGACellXY, "NLGACellXY");
    ExportValue(node, fLGAThickness, "LGAThickness");
    ExportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ExportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
}

} // namespace Musae::Detector::Description
