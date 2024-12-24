#pragma once

#include "Mustard/Detector/Description/DescriptionWithCacheBase.h++"

#include "CLHEP/Geometry/Transform3D.h"

#include "muc/array"

#include <map>
#include <unordered_map>
#include <vector>

namespace Musae::Detector::Description {

class LGA final : public Mustard::Detector::Description::DescriptionWithCacheBase<LGA> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    LGA();
    ~LGA() = default;

public:
    // Geometry

    auto Position() const -> auto { return *fPosition; }
    auto EulerAngleAlpha() const -> auto { return *fEulerAngleAlpha; }
    auto EulerAngleBeta() const -> auto { return *fEulerAngleBeta; }
    auto EulerAngleGamma() const -> auto { return *fEulerAngleGamma; }
    auto NModule() const -> auto { return *fNModule; }
    auto ModuleSpacing() const -> auto { return *fModuleSpacing; }
    auto ScintillatorWidth() const -> auto { return *fScintillatorWidth; }
    auto ScintillatorThickness() const -> auto { return *fScintillatorThickness; }
    auto UseFastLGA() const -> auto { return *fUseFastLGA; }
    auto LGACellWidth() const -> auto { return *fLGACellWidth; }
    auto NLGACellX() const -> auto { return *fNLGACellX; }
    auto NLGACellY() const -> auto { return *fNLGACellY; }
    auto LGAThickness() const -> auto { return *fLGAThickness; }

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
    auto NLGACellX(int val) -> void { fNLGACellX = val; }
    auto NLGACellY(int val) -> void { fNLGACellY = val; }
    auto LGAThickness(double val) -> void { fLGAThickness = val; }

    auto Rotation() const -> const auto& { return *fRotation; }
    auto Transform(double zLocal) const -> HepGeom::Transform3D;
    auto Transform(int moduleID, double zShift = 0) const -> HepGeom::Transform3D;
    auto FiberX(int fiberLocalID) const -> double;
    auto FiberY(int fiberLocalID) const -> double;

    // Detection

    auto EnergyDepositionThreshold() const -> auto { return *fEnergyDepositionThreshold; }
    auto TimeResolutionFWHM() const -> auto { return *fTimeResolutionFWHM; }

    auto EnergyDepositionThreshold(double val) -> void { fEnergyDepositionThreshold = val; }
    auto TimeResolutionFWHM(double val) -> void { fTimeResolutionFWHM = val; }

    // Digitization

    auto NChannelPerChip() const -> auto { return *fNChannelPerChip; }
    auto CoincidenceTimeWindow() const -> auto { return *fCoincidenceTimeWindow; }

    auto NChannelPerChip(int val) -> void { fNChannelPerChip = val; }
    auto CoincidenceTimeWindow(double val) -> void { fCoincidenceTimeWindow = val; }

    struct BasicChInfo {
        int moduleID;
        char edge; // 'x' or 'y'
        int fiberLocalID;
        constexpr auto operator<=>(const BasicChInfo&) const = default;
    };

    struct ChInfo : BasicChInfo {
        int chipID;
        double edgePosition;
    };

    auto ChannelInfo(int channelID) const -> const ChInfo&;
    auto Intersection(int chID1, int chID2) const -> muc::array2d;

    // Analysis

    auto NLuminousFiberThresholdPerDirection() const -> auto { return *fNLuminousFiberThresholdPerDirection; }

    auto NLuminousFiberThresholdPerDirection(int val) -> void { fNLuminousFiberThresholdPerDirection = val; }

private:
    // Geometry

    auto CalculateRotation() const -> HepGeom::Rotate3D;

    // Digitization

    auto CalculateInverseChipMap() const -> std::vector<int>;
    auto CalculateInverseChannelMap() const -> std::map<BasicChInfo, int>;
    auto CalculateChannelInfo() const -> std::unordered_map<int, ChInfo>;

    auto CheckModuleIDFromChipMap(int moduleID) const -> void;
    auto CheckEdgeFiberIDFromChannelMap(char edge, int fiberLocalID) const -> void;

    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    // Geometry
    Simple<muc::array3d> fPosition;
    Simple<double> fEulerAngleAlpha;
    Simple<double> fEulerAngleBeta;
    Simple<double> fEulerAngleGamma;
    Simple<int> fNModule;
    Simple<double> fModuleSpacing;
    Simple<double> fScintillatorWidth;
    Simple<double> fScintillatorThickness;
    Simple<bool> fUseFastLGA;
    Simple<double> fLGACellWidth;
    Simple<int> fNLGACellX;
    Simple<int> fNLGACellY;
    Simple<double> fLGAThickness;
    Cached<HepGeom::Rotate3D> fRotation;
    // Detection
    Simple<double> fEnergyDepositionThreshold;
    Simple<double> fTimeResolutionFWHM;
    // Digitization
    Simple<int> fNChannelPerChip;
    Simple<std::unordered_map<int, int>> fChipMap;                              // chipID -> moduleID
    Simple<std::unordered_map<int, std::pair<char, int>>> fPerModuleChannelMap; // moduleChannelID -> {edge, fiberLocalID}
    Simple<double> fCoincidenceTimeWindow;
    Cached<std::vector<int>> fInverseChipMap;              // moduleID -> chipID
    Cached<std::map<BasicChInfo, int>> fInverseChannelMap; // {moduleID, edge, fiberLocalID} -> channelID
    Cached<std::unordered_map<int, ChInfo>> fChannelInfo;  // channelID -> channel info
    // Analysis
    Simple<int> fNLuminousFiberThresholdPerDirection;
};

} // namespace Musae::Detector::Description
