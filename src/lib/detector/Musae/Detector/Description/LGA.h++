#pragma once

#include "Mustard/Detector/Description/DescriptionWithCacheBase.h++"

#include "CLHEP/Geometry/Transform3D.h"

#include "muc/array"
#include "muc/btree_map"
#include "muc/hash_map"

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
    auto ScintillatorWidthX() const -> auto { return *fScintillatorWidthX; }
    auto ScintillatorWidthY() const -> auto { return *fScintillatorWidthY; }
    auto ScintillatorThickness() const -> auto { return *fScintillatorThickness; }
    auto UseFastLGA() const -> auto { return *fUseFastLGA; }
    auto LGACellWidth() const -> auto { return *fLGACellWidth; }
    auto NFiberX() const -> auto { return *fNFiberX; }
    auto NFiberY() const -> auto { return *fNFiberY; }
    auto LGAThickness() const -> auto { return *fLGAThickness; }

    auto Position(muc::array3d val) -> void { fPosition = val; }
    auto EulerAngleAlpha(double val) -> void { fEulerAngleAlpha = val; }
    auto EulerAngleBeta(double val) -> void { fEulerAngleBeta = val; }
    auto EulerAngleGamma(double val) -> void { fEulerAngleGamma = val; }
    auto NModule(int val) -> void { fNModule = val; }
    auto ModuleSpacing(double val) -> void { fModuleSpacing = val; }
    auto ScintillatorWidthX(double val) -> void { fScintillatorWidthX = val; }
    auto ScintillatorWidthY(double val) -> void { fScintillatorWidthY = val; }
    auto ScintillatorThickness(double val) -> void { fScintillatorThickness = val; }
    auto UseFastLGA(bool val) -> void { fUseFastLGA = val; }
    auto LGACellWidth(double val) -> void { fLGACellWidth = val; }
    auto NFiberX(int val) -> void { fNFiberX = val; }
    auto NFiberY(int val) -> void { fNFiberY = val; }
    auto LGAThickness(double val) -> void { fLGAThickness = val; }

    auto LGAWidthX() const -> auto { return fNFiberX * fLGACellWidth; }
    auto LGAWidthY() const -> auto { return fNFiberY * fLGACellWidth; }
    auto ModuleZ(int moduleID) const -> auto { return moduleID * fModuleSpacing; }
    auto Rotation() const -> const auto& { return *fRotation; }
    auto Transform(double zLocal) const -> HepGeom::Transform3D;
    auto Transform(int moduleID, double zShift = 0) const -> HepGeom::Transform3D;
    auto FiberX(int fiberLocalID) const -> double;
    auto FiberY(int fiberLocalID) const -> double;

    // Material

    auto ScintillationTimeConstant1() const -> auto { return *fScintillationTimeConstant1; }

    auto ScintillationTimeConstant1(double val) -> void { fScintillationTimeConstant1 = val; }

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

    auto ModuleID(int chipID) const -> int;
    auto ChipID(int moduleID) const -> int;
    auto ChannelInfo() const -> const auto& { return *fChannelInfo; }
    auto ChannelInfo(int channelID) const -> const ChInfo&;
    auto TryChannelInfo(int channelID) const -> const ChInfo*;
    auto Intersection(int chID1, int chID2) const -> muc::array2d;

    // Analysis

    auto LuminousDigiEnergyThreshold() const -> auto { return *fLuminousDigiEnergyThreshold; }
    auto NLuminousDigiThresholdPerDirection() const -> auto { return *fNLuminousDigiThresholdPerDirection; }
    auto NHitThreshold() const -> auto { return *fNHitThreshold; }
    auto SoftDeadTime() const -> auto { return *fSoftDeadTime; }

    auto LuminousDigiEnergyThreshold(double val) -> void { fLuminousDigiEnergyThreshold = val; }
    auto NLuminousDigiThresholdPerDirection(int val) -> void { fNLuminousDigiThresholdPerDirection = val; }
    auto NHitThreshold(int val) -> void { fNHitThreshold = val; }
    auto SoftDeadTime(double val) -> void { fSoftDeadTime = val; }

private:
    // Geometry

    auto CalculateRotation() const -> HepGeom::Rotate3D;

    // Digitization

    auto CalculateInverseChipMap() const -> std::vector<int>;
    auto CalculateInverseChannelMap() const -> muc::btree_map<BasicChInfo, int>;
    auto CalculateChannelInfo() const -> muc::flat_hash_map<int, ChInfo>;

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
    Simple<double> fScintillatorWidthX;
    Simple<double> fScintillatorWidthY;
    Simple<double> fScintillatorThickness;
    Simple<bool> fUseFastLGA;
    Simple<double> fLGACellWidth;
    Simple<int> fNFiberX;
    Simple<int> fNFiberY;
    Simple<double> fLGAThickness;
    Cached<HepGeom::Rotate3D> fRotation;
    // Material
    Simple<double> fScintillationTimeConstant1;
    // Detection
    Simple<double> fEnergyDepositionThreshold;
    Simple<double> fTimeResolutionFWHM;
    // Digitization
    Simple<int> fNChannelPerChip;
    Simple<std::unordered_map<int, int>> fChipMap;                              // chipID -> moduleID
    Simple<std::unordered_map<int, std::pair<char, int>>> fPerModuleChannelMap; // moduleChannelID -> {edge, fiberLocalID}
    Simple<double> fCoincidenceTimeWindow;
    Cached<std::vector<int>> fInverseChipMap;                    // moduleID -> chipID
    Cached<muc::btree_map<BasicChInfo, int>> fInverseChannelMap; // {moduleID, edge, fiberLocalID} -> channelID
    Cached<muc::flat_hash_map<int, ChInfo>> fChannelInfo;        // channelID -> channel info
    // Analysis
    Simple<double> fLuminousDigiEnergyThreshold;
    Simple<int> fNLuminousDigiThresholdPerDirection;
    Simple<int> fNHitThreshold;
    Simple<double> fSoftDeadTime;
};

} // namespace Musae::Detector::Description
