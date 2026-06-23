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
#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/VectorCast.h++"

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
    fPosition{this, {{0, 0, 0}}},
    fEulerAngleAlpha{this, {0.}},
    fEulerAngleBeta{this, {0.}},
    fEulerAngleGamma{this, {0.}},
    fNModule{this, 3},
    fModuleSpacing{this, 14_cm},
    fScintillatorWidthX{this, 30_cm},
    fScintillatorWidthY{this, 30_cm},
    fScintillatorThickness{this, 1_cm},
    fUseFastLGA{this, true},
    fLGACellWidth{this, 1.1_cm},
    fNFiberX{this, 27},
    fNFiberY{this, 27},
    fLGAThickness{this, 1_cm},
    fRotation{this, [this] { return CalculateRotations(); }},
    // Material
    fScintillationTimeConstant1{this, 2.4_ns},
    // Detection
    fEnergyDepositionThreshold{this, 10_keV},
    fTimeResolutionFWHM{this, 1_ns},
    // Digitization
    fNChannelPerChip{this, 64},
    fChipMap{this, {}},
    fPerModuleChannelMap{this, {}},
    fCoincidenceTimeWindow{this, 40_ns},
    fInverseChipMap{this, [this] { return CalculateInverseChipMap(); }},
    fInverseChannelMap{this, [this] { return CalculateInverseChannelMap(); }},
    fChannelInfo{this, [this] { return CalculateChannelInfo(); }},
    // Analysis
    fLuminousDigiEnergyThreshold{this, 0.},
    fNLuminousDigiThresholdPerDirection{this, 3},
    fNHitThreshold{this, 3},
    fNHitPreThreshold{this, 1},
    fNSymfilterThreshold{this, 1},
    fSoftDeadTime{this, 0.} {
    // Initialize map
    fChipMap = {
        {5, 0},
        {0, 1},
        {4, 2}
    };
    fPerModuleChannelMap = {
        {16, {'x', 11}},
        {17, {'x', 9} },
        {18, {'x', 10}},
        {19, {'x', 8} },
        {20, {'x', 6} },
        {21, {'x', 7} },
        {22, {'x', 4} },
        {23, {'x', 5} },
        {24, {'x', 2} },
        {25, {'x', 3} },
        {26, {'x', 1} },
        {28, {'x', 0} },
        {29, {'x', 12}},
        {30, {'x', 13}},
        {31, {'x', 14}},
        {32, {'x', 26}},
        {33, {'x', 24}},
        {34, {'x', 25}},
        {35, {'x', 22}},
        {36, {'x', 23}},
        {37, {'x', 21}},
        {40, {'x', 18}},
        {41, {'x', 19}},
        {42, {'x', 17}},
        {44, {'x', 16}},
        {46, {'x', 15}},
        {39, {'x', 20}},
        {0,  {'y', 16}},
        {1,  {'y', 15}},
        {2,  {'y', 13}},
        {3,  {'y', 18}},
        {4,  {'y', 26}},
        {5,  {'y', 20}},
        {6,  {'y', 25}},
        {7,  {'y', 22}},
        {8,  {'y', 23}},
        {9,  {'y', 2} },
        {10, {'y', 17}},
        {11, {'y', 12}},
        {12, {'y', 19}},
        {13, {'y', 14}},
        {14, {'y', 21}},
        {15, {'y', 24}},
        {27, {'y', 0} },
        {38, {'y', 1} },
        {50, {'y', 7} },
        {51, {'y', 9} },
        {52, {'y', 8} },
        {53, {'y', 11}},
        {54, {'y', 10}},
        {59, {'y', 6} },
        {58, {'y', 4} },
        {62, {'y', 3} },
        {61, {'y', 5} }
    };
}

auto LGA::EnvelopeTransform(int detID) const -> HepGeom::Transform3D {
    return HepGeom::Translate3D{Mustard::VectorCast<CLHEP::Hep3Vector>(Position(detID))} * Rotation(detID);
}

auto LGA::ModuleLocalTransform(int moduleID, double zShift) const -> HepGeom::Transform3D {
    return HepGeom::Translate3D{0, 0, ModuleZ(moduleID) + zShift - BoxCenterZ()};
}

auto LGA::Transform(int detID, int moduleID, double zShift) const -> HepGeom::Transform3D {
    double normalOffset = ModuleZ(moduleID) + zShift;

    HepGeom::Vector3D<double> normalDir(0, 0, normalOffset);
    HepGeom::Vector3D<double> rotatedNormal = Rotation(detID) * normalDir;

    const auto& pos{Position(detID)};
    return HepGeom::Translate3D{
               pos[0] + rotatedNormal.x(),
               pos[1] + rotatedNormal.y(),
               pos[2] + rotatedNormal.z()} *
           Rotation(detID);
}

auto LGA::HitWorldPosition(int detID, int moduleID, float lx, float ly, double zShift) const -> muc::array3d {
    if (zShift == -1) {
        zShift = -ScintillatorThickness() / 2.;
    }
    // offset from detector Position to the scintillator plane in local z
    const double offset{ModuleZ(moduleID) + zShift - BoxCenterZ()};
    // rotate local (lx, ly, offset) by detector rotation and add world Position
    const HepGeom::Vector3D<double> localPoint{lx, ly, offset};
    const HepGeom::Vector3D<double> rotated{Rotation(detID) * localPoint};
    const auto& pos{Position(detID)};
    return {pos[0] + rotated.x(),
            pos[1] + rotated.y(),
            pos[2] + rotated.z()};
}

auto LGA::FiberX(int fiberLocalID) const -> double {
    return LGACellWidth() * (2 * fiberLocalID + 1 - NFiberX()) / 2;
}

auto LGA::FiberY(int fiberLocalID) const -> double {
    return LGACellWidth() * (2 * fiberLocalID + 1 - NFiberY()) / 2;
}

auto LGA::ModuleID(int chipID) const -> int {
    try {
        return fChipMap->at(chipID);
    } catch (const std::out_of_range&) {
        Mustard::Throw<std::out_of_range>(fmt::format("Chip ID {} not found in chip map", chipID));
    }
}

auto LGA::ChipID(int moduleID) const -> int {
    try {
        return fInverseChipMap->at(moduleID);
    } catch (const std::out_of_range&) {
        Mustard::Throw<std::out_of_range>(fmt::format("Module ID {} not found in chip map", moduleID));
    }
}

auto LGA::ChannelInfo(int channelID) const -> const ChInfo& {
    if (const auto chInfo{TryChannelInfo(channelID)}) {
        return *chInfo;
    }
    Mustard::Throw<std::out_of_range>(fmt::format("Channel ID {} not found in channel map", channelID));
}

auto LGA::TryChannelInfo(int channelID) const -> const ChInfo* {
    const auto i{fChannelInfo->find(channelID)};
    if (i == fChannelInfo->cend()) {
        return nullptr;
    }
    return &i->second;
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

auto LGA::CalculateRotations() const -> std::vector<HepGeom::Rotate3D> {
    std::vector<HepGeom::Rotate3D> rotations;
    for (int i{}; i < NDetector(); ++i) {
        rotations.emplace_back(CLHEP::HepRotation{(*fEulerAngleAlpha)[i], (*fEulerAngleBeta)[i], (*fEulerAngleGamma)[i]});
    }
    return rotations;
}

auto LGA::CalculateInverseChipMap() const -> std::vector<int> {
    if (ssize(*fChipMap) != NModule()) {
        Mustard::Throw<std::runtime_error>(fmt::format("Chip map has size of {}, but it should be of size {}", fChipMap->size(), NModule()));
    }

    muc::flat_hash_map<int, int> inverseMap;
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

auto LGA::CalculateInverseChannelMap() const -> muc::btree_map<BasicChInfo, int> {
    if (ssize(*fPerModuleChannelMap) != NFiberX() + NFiberY()) {
        Mustard::Throw<std::runtime_error>(fmt::format("Per module channel map has size of {}, but it should be of size {}+{}={}",
                                                       fPerModuleChannelMap->size(), NFiberX(), NFiberY(), NFiberX() + NFiberY()));
    }

    muc::btree_map<BasicChInfo, int> inverseMap;
    for (int moduleID{}; moduleID < NModule(); ++moduleID) {
        for (auto&& [moduleChannelID, edgeAndEdgeFiberID] : std::as_const(*fPerModuleChannelMap)) {
            const auto& [edge, fiberLocalID]{edgeAndEdgeFiberID};
            CheckEdgeFiberIDFromChannelMap(edge, fiberLocalID);
            auto channelID{ChipID(moduleID) * NChannelPerChip() + moduleChannelID};
            inverseMap[{moduleID, edge, fiberLocalID}] = channelID;
        }
    }

    if (ssize(inverseMap) != NModule() * (NFiberX() + NFiberY())) {
        Mustard::Throw<std::runtime_error>("Channel map is not a bijection");
    }
    return inverseMap;
}

auto LGA::CalculateChannelInfo() const -> muc::flat_hash_map<int, ChInfo> {
    muc::flat_hash_map<int, ChInfo> channelInfo;
    for (int moduleID{}; moduleID < NModule(); ++moduleID) {
        const auto chipID{fInverseChipMap->at(moduleID)};
        for (int i{}; i < NFiberX(); ++i) {
            const BasicChInfo basicChInfo{moduleID, 'x', i};
            const auto channelID{fInverseChannelMap->at(basicChInfo)};
            channelInfo[channelID] = {basicChInfo, chipID, FiberX(i)};
        }
        for (int j{}; j < NFiberY(); ++j) {
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
        if (fiberLocalID >= NFiberX()) {
            Mustard::Throw<std::runtime_error>(fmt::format("Fiber ID {} along x direction in channel map is out of range", fiberLocalID));
        }
        break;
    case 'y':
        if (fiberLocalID >= NFiberY()) {
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
    ImportValue(node, fScintillatorWidthX, "ScintillatorWidthX");
    ImportValue(node, fScintillatorWidthY, "ScintillatorWidthY");
    ImportValue(node, fScintillatorThickness, "ScintillatorThickness");
    ImportValue(node, fUseFastLGA, "UseFastLGA");
    ImportValue(node, fLGACellWidth, "LGACellWidth");
    ImportValue(node, fNFiberX, "NFiberX");
    ImportValue(node, fNFiberY, "NFiberY");
    ImportValue(node, fLGAThickness, "LGAThickness");
    ImportValue(node, fScintillationTimeConstant1, "ScintillationTimeConstant1");
    ImportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ImportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
    ImportValue(node, fNChannelPerChip, "NChannelPerChip");
    ImportValue(node, fCoincidenceTimeWindow, "CoincidenceTimeWindow");
    ImportValue(node, fChipMap, "ChipMap");
    ImportValue(node, fPerModuleChannelMap, "PerModuleChannelMap");
    ImportValue(node, fLuminousDigiEnergyThreshold, "LuminousDigiEnergyThreshold");
    ImportValue(node, fNLuminousDigiThresholdPerDirection, "NLuminousDigiThresholdPerDirection");
    ImportValue(node, fNHitThreshold, "NHitThreshold");
    ImportValue(node, fNHitPreThreshold, "NHitPreThreshold");
    ImportValue(node, fNSymfilterThreshold, "NSymfilterThreshold");
    ImportValue(node, fSoftDeadTime, "SoftDeadTime");
}

auto LGA::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fPosition, "Position");
    ExportValue(node, fEulerAngleAlpha, "EulerAngleAlpha");
    ExportValue(node, fEulerAngleBeta, "EulerAngleBeta");
    ExportValue(node, fEulerAngleGamma, "EulerAngleGamma");
    ExportValue(node, fNModule, "NModule");
    ExportValue(node, fModuleSpacing, "ModuleSpacing");
    ExportValue(node, fScintillatorWidthX, "ScintillatorWidthX");
    ExportValue(node, fScintillatorWidthY, "ScintillatorWidthY");
    ExportValue(node, fScintillatorThickness, "ScintillatorThickness");
    ExportValue(node, fUseFastLGA, "UseFastLGA");
    ExportValue(node, fLGACellWidth, "LGACellWidth");
    ExportValue(node, fNFiberX, "NFiberX");
    ExportValue(node, fNFiberY, "NFiberY");
    ExportValue(node, fLGAThickness, "LGAThickness");
    ExportValue(node, fScintillationTimeConstant1, "ScintillationTimeConstant1");
    ExportValue(node, fEnergyDepositionThreshold, "EnergyDepositionThreshold");
    ExportValue(node, fTimeResolutionFWHM, "TimeResolutionFWHM");
    ExportValue(node, fNChannelPerChip, "NChannelPerChip");
    ExportValue(node, fCoincidenceTimeWindow, "CoincidenceTimeWindow");
    ExportValue(node, fChipMap, "ChipMap");
    ExportValue(node, fPerModuleChannelMap, "PerModuleChannelMap");
    ExportValue(node, fLuminousDigiEnergyThreshold, "LuminousDigiEnergyThreshold");
    ExportValue(node, fNLuminousDigiThresholdPerDirection, "NLuminousDigiThresholdPerDirection");
    ExportValue(node, fNHitThreshold, "NHitThreshold");
    ExportValue(node, fNHitPreThreshold, "NHitPreThreshold");
    ExportValue(node, fNSymfilterThreshold, "NSymfilterThreshold");
    ExportValue(node, fSoftDeadTime, "SoftDeadTime");
}

} // namespace Musae::Detector::Description
