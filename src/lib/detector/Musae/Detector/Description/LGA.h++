#pragma once

#include "Mustard/Detector/Description/DescriptionBase.h++"

#include "CLHEP/Geometry/Transform3D.h"

#include "muc/array"

namespace Musae::Detector::Description {

class LGA final : public Mustard::Detector::Description::DescriptionBase<LGA> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    LGA();
    ~LGA() = default;

public:
    // Geometry

    auto Position() const -> auto { return fPosition; }
    auto EulerAngleAlpha() const -> auto { return fEulerAngleAlpha; }
    auto EulerAngleBeta() const -> auto { return fEulerAngleBeta; }
    auto EulerAngleGamma() const -> auto { return fEulerAngleGamma; }
    auto NModule() const -> auto { return fNModule; }
    auto ModuleSpacing() const -> auto { return fModuleSpacing; }
    auto ScintillatorWidth() const -> auto { return fScintillatorWidth; }
    auto ScintillatorThickness() const -> auto { return fScintillatorThickness; }
    auto UseFastLGA() const -> auto { return fUseFastLGA; }
    auto LGACellWidth() const -> auto { return fLGACellWidth; }
    auto NLGACellXY() const -> auto { return fNLGACellXY; }
    auto LGAThickness() const -> auto { return fLGAThickness; }

    auto Position(muc::array3d val) -> void { fPosition = val; }
    auto EulerAngleAlpha(double val) -> void { fEulerAngleAlpha = val; }
    auto EulerAngleBeta(double val) -> void { fEulerAngleBeta = val; }
    auto EulerAngleGamma(double val) -> void { fEulerAngleGamma = val; }
    auto NModule(int val) -> void { fNModule = val; }
    auto ModuleSpacing(double val) -> void { fModuleSpacing = val; }
    auto ScintillatorWidth(double val) -> void { fScintillatorWidth = val; }
    auto ScintillatorThickness(double val) -> void { fScintillatorThickness = val; }
    auto UseFastLGA(bool val) -> void { fUseFastLGA = val; }
    auto LGACellWidth(double val) -> void { fLGACellWidth = val; }
    auto NLGACellXY(int val) -> void { fNLGACellXY = val; }
    auto LGAThickness(double val) -> void { fLGAThickness = val; }

    auto Transform(int id, double zShift = 0) const -> HepGeom::Transform3D;

    // Detection

    auto EnergyDepositionThreshold() const -> auto { return fEnergyDepositionThreshold; }
    auto TimeResolutionFWHM() const -> auto { return fTimeResolutionFWHM; }

    auto EnergyDepositionThreshold(double val) -> void { fEnergyDepositionThreshold = val; }
    auto TimeResolutionFWHM(double val) -> void { fTimeResolutionFWHM = val; }

private:
    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    // Geometry
    muc::array3d fPosition;
    double fEulerAngleAlpha;
    double fEulerAngleBeta;
    double fEulerAngleGamma;
    int fNModule;
    double fModuleSpacing;
    double fScintillatorWidth;
    double fScintillatorThickness;
    bool fUseFastLGA;
    double fLGACellWidth;
    int fNLGACellXY;
    double fLGAThickness;
    // Detection
    double fEnergyDepositionThreshold;
    double fTimeResolutionFWHM;
};

} // namespace Musae::Detector::Description
