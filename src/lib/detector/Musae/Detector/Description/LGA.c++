#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Vector/EulerAngles.h"
#include "CLHEP/Vector/Rotation.h"

#include "muc/ctype"
#include "muc/math"

#include "fmt/core.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Energy;
using namespace Mustard::LiteralUnit::Length;
using namespace Mustard::LiteralUnit::Time;

LGA::LGA() : // clang-format off
    DescriptionWithCacheBase{"LGA"}, // clang-format on
    // Geometry
    fPosition{this, {0, 0, 0}},
    fEulerAngleAlpha{this, 0.},
    fEulerAngleBeta{this, 0.},
    fEulerAngleGamma{this, 0.},
    fNModule{this, 3},
    fModuleSpacing{this, 15_cm},
    fScintillatorWidth{this, 30_cm},
    fScintillatorThickness{this, 1_cm},
    fUseFastLGA{this, true},
    fLGACellWidth{this, 1_cm},
    fNLGACellX{this, 27},
    fNLGACellY{this, 27},
    fLGAThickness{this, 1_cm},
    fRotation{this, [this] { return CalculateRotation(); }},
    // Detection
    fEnergyDepositionThreshold{this, 10_keV},
    fTimeResolutionFWHM{this, 1_ns},
    // Digitization
    fNChannelPerChip{this, 64},
    fChipMap{this, {}},
    fPerModuleChannelMap{this, {}},
    fCoincidenceTimeWindow{this, 30_ns},
    fInverseChipMap{this, [this] { return CalculateInverseChipMap(); }},
    fInverseChannelMap{this, [this] { return CalculateInverseChannelMap(); }},
    fChannelInfo{this, [this] { return CalculateChannelInfo(); }},
    // Analysis
    fLuminousDigiEnergyThreshold{this, 0.},
    fNLuminousDigiThresholdPerDirection{this, 3},
    fNHitThreshold{this, 3} {
    // Initialize map
    fChipMap = {
        {0, 0},
        {5, 1},
        {4, 2}
    };
    fPerModuleChannelMap = {
        {16, {'x', 0} },
        {17, {'x', 1} },
        {18, {'x', 2} },
        {19, {'x', 3} },
        {20, {'x', 4} },
        {21, {'x', 5} },
        {22, {'x', 6} },
        {23, {'x', 7} },
        {24, {'x', 8} },
        {25, {'x', 9} },
        {26, {'x', 10}},
        {28, {'x', 11}},
        {29, {'x', 12}},
        {30, {'x', 13}},
        {31, {'x', 14}},
        {32, {'x', 15}},
        {33, {'x', 16}},
        {34, {'x', 17}},
        {35, {'x', 18}},
        {36, {'x', 19}},
        {37, {'x', 20}},
        {40, {'x', 21}},
        {41, {'x', 22}},
        {42, {'x', 23}},
        {44, {'x', 24}},
        {46, {'x', 25}},
        {39, {'x', 26}},
        {0,  {'y', 0} },
        {1,  {'y', 1} },
        {2,  {'y', 2} },
        {3,  {'y', 3} },
        {4,  {'y', 4} },
        {5,  {'y', 5} },
        {6,  {'y', 6} },
        {7,  {'y', 7} },
        {8,  {'y', 8} },
        {9,  {'y', 9} },
        {10, {'y', 10}},
        {11, {'y', 11}},
        {12, {'y', 12}},
        {13, {'y', 13}},
        {14, {'y', 14}},
        {15, {'y', 15}},
        {27, {'y', 16}},
        {38, {'y', 17}},
        {50, {'y', 18}},
        {51, {'y', 19}},
        {52, {'y', 20}},
        {53, {'y', 21}},
        {54, {'y', 22}},
        {59, {'y', 23}},
        {58, {'y', 24}},
        {62, {'y', 25}},
        {61, {'y', 26}}
    };
}

auto LGA::Transform(double zLocal) const -> HepGeom::Transform3D {
    return HepGeom::Translate3D{Position()[0], Position()[1], zLocal} * Rotation();
}

auto LGA::Transform(int moduleID, double zShift) const -> HepGeom::Transform3D {
    return Transform(Position()[2] + moduleID * fModuleSpacing + zShift);
}

auto LGA::FiberX(int fiberLocalID) const -> double {
    return LGACellWidth() * (2 * fiberLocalID + 1 - NLGACellX()) / 2;
}

auto LGA::FiberY(int fiberLocalID) const -> double {
    return LGACellWidth() * (2 * fiberLocalID + 1 - NLGACellY()) / 2;
}

auto LGA::ChannelInfo(int channelID) const -> const ChInfo& {
    try {
        return fChannelInfo->at(channelID);
    } catch (const std::out_of_range&) {
        Mustard::Throw<std::out_of_range>(fmt::format("Channel ID {} not found in channel map", channelID));
    }
}

auto LGA::Intersection(int chID1, int chID2) const -> muc::array2d {
    const auto& chInfo1{ChannelInfo(chID1)};
    const auto& chInfo2{ChannelInfo(chID2)};
    if (chInfo1.moduleID != chInfo2.moduleID) {
        Mustard::Throw<std::invalid_argument>(fmt::format("Channel {} and {} are not in the same module", chID1, chID2));
    }
    const auto edge1{muc::tolower(chInfo1.edge)};
    const auto edge2{muc::tolower(chInfo2.edge)};
    if (edge1 == edge2) {
        Mustard::Throw<std::invalid_argument>(fmt::format("Channel {} and {} are of the same direction", chID1, chID2));
    }
    if (edge1 == 'x') {
        return {chInfo1.edgePosition,
                chInfo2.edgePosition};
    } else { // edge2 == 'x'
        return {chInfo2.edgePosition,
                chInfo1.edgePosition};
    }
}

auto LGA::CalculateRotation() const -> HepGeom::Rotate3D {
    return CLHEP::HepRotation{fEulerAngleAlpha, fEulerAngleBeta, fEulerAngleGamma};
}

auto LGA::CalculateInverseChipMap() const -> std::vector<int> {
    if (ssize(*fChipMap) != NModule()) {
        Mustard::Throw<std::runtime_error>(fmt::format("Chip map has size of {}, but it should be of size {}", fChipMap->size(), NModule()));
    }

    std::unordered_map<int, int> inverseMap;
    for (auto&& [chipID, moduleID] : std::as_const(*fChipMap)) {
        CheckModuleIDFromChipMap(moduleID);
        inverseMap[moduleID] = chipID;
    }
    if (ssize(inverseMap) != NModule()) {
        Mustard::Throw<std::runtime_error>("Chip map is not a bijection");
    }

    std::vector<int> result(NModule());
    for (int moduleID{}; moduleID < NModule(); ++moduleID) {
        result[moduleID] = inverseMap.at(moduleID);
    }
    return result;
}

auto LGA::CalculateInverseChannelMap() const -> std::map<BasicChInfo, int> {
    if (ssize(*fPerModuleChannelMap) != NLGACellX() + NLGACellY()) {
        Mustard::Throw<std::runtime_error>(fmt::format("Per module channel map has size of {}, but it should be of size {}+{}={}",
                                                       fPerModuleChannelMap->size(), NLGACellX(), NLGACellY(), NLGACellX() + NLGACellY()));
    }

    std::map<BasicChInfo, int> inverseMap;
    for (int moduleID{}; moduleID < NModule(); ++moduleID) {
        const auto chipID{[&] {
            try {
                return fInverseChipMap->at(moduleID);
            } catch (const std::out_of_range&) {
                Mustard::Throw<std::out_of_range>(fmt::format("Module ID {} not found in chip map", moduleID));
            }
        }()};
        for (auto&& [moduleChannelID, edgeAndEdgeFiberID] : std::as_const(*fPerModuleChannelMap)) {
            const auto& [edge, fiberLocalID]{edgeAndEdgeFiberID};
            CheckEdgeFiberIDFromChannelMap(edge, fiberLocalID);
            auto channelID{chipID * NChannelPerChip() + moduleChannelID};
            inverseMap[{moduleID, edge, fiberLocalID}] = channelID;
        }
    }

    if (ssize(inverseMap) != NModule() * (NLGACellX() + NLGACellY())) {
        Mustard::Throw<std::runtime_error>("Channel map is not a bijection");
    }
    return inverseMap;
}

auto LGA::CalculateChannelInfo() const -> std::unordered_map<int, ChInfo> {
    std::unordered_map<int, ChInfo> channelInfo;
    for (int moduleID{}; moduleID < NModule(); ++moduleID) {
        const auto chipID{fInverseChipMap->at(moduleID)};
        for (int i{}; i < NLGACellX(); ++i) {
            const BasicChInfo basicChInfo{moduleID, 'x', i};
            const auto channelID{fInverseChannelMap->at(basicChInfo)};
            channelInfo[channelID] = {basicChInfo, chipID, FiberX(i)};
        }
        for (int j{}; j < NLGACellY(); ++j) {
            const BasicChInfo basicChInfo{moduleID, 'y', j};
            const auto channelID{fInverseChannelMap->at(basicChInfo)};
            channelInfo[channelID] = {basicChInfo, chipID, FiberY(j)};
        }
    }
    return channelInfo;
}

auto LGA::CheckModuleIDFromChipMap(int moduleID) const -> void {
    if (moduleID >= NModule()) {
        Mustard::Throw<std::runtime_error>(fmt::format("Module ID {} in chip map is out of range", moduleID));
    }
}

auto LGA::CheckEdgeFiberIDFromChannelMap(char edge, int fiberLocalID) const -> void {
    switch (edge) {
    case 'x':
        if (fiberLocalID >= NLGACellX()) {
            Mustard::Throw<std::runtime_error>(fmt::format("Fiber ID {} along x direction in channel map is out of range", fiberLocalID));
        }
        break;
    case 'y':
        if (fiberLocalID >= NLGACellY()) {
            Mustard::Throw<std::runtime_error>(fmt::format("Fiber ID {} along y direction in channel map is out of range", fiberLocalID));
        }
        break;
    default:
        Mustard::Throw<std::runtime_error>(fmt::format("Invalid edge '{}'", edge));
        break;
    }
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
    ImportValue(node, fNLGACellX, "NLGACellX");
    ImportValue(node, fNLGACellY, "NLGACellY");
    ImportValue(node, fLGAThickness, "LGAThickness");
    ImportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ImportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
    ImportValue(node, fNChannelPerChip, "NChannelPerChip");
    ImportValue(node, fCoincidenceTimeWindow, "CoincidenceTimeWindow");
    ImportValue(node, fChipMap, "ChipMap");
    ImportValue(node, fPerModuleChannelMap, "PerModuleChannelMap");
    ImportValue(node, fLuminousDigiEnergyThreshold, "LuminousDigiEnergyThreshold");
    ImportValue(node, fNLuminousDigiThresholdPerDirection, "NLuminousDigiThresholdPerDirection");
    ImportValue(node, fNHitThreshold, "NHitThreshold");
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
    ExportValue(node, fNLGACellX, "NLGACellX");
    ExportValue(node, fNLGACellY, "NLGACellY");
    ExportValue(node, fLGAThickness, "LGAThickness");
    ExportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ExportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
    ExportValue(node, fNChannelPerChip, "NChannelPerChip");
    ExportValue(node, fCoincidenceTimeWindow, "CoincidenceTimeWindow");
    ExportValue(node, fChipMap, "ChipMap");
    ExportValue(node, fPerModuleChannelMap, "PerModuleChannelMap");
    ExportValue(node, fLuminousDigiEnergyThreshold, "LuminousDigiEnergyThreshold");
    ExportValue(node, fNLuminousDigiThresholdPerDirection, "NLuminousDigiThresholdPerDirection");
    ExportValue(node, fNHitThreshold, "NHitThreshold");
}

} // namespace Musae::Detector::Description
