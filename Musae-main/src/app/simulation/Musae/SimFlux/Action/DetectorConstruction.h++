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
#pragma once

#include "Musae/Detector/Description/Hierarchy.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/Detector/Description/Material.h++"
#include "Musae/Detector/Description/Model.h++"
#include "Musae/Detector/Description/Terrain.h++"
#include "Musae/Detector/Description/World.h++"



#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4VUserDetectorConstruction.hh"

#include <memory>

namespace Mustard::Detector::Definition {
class DefinitionBase;
} // namespace Mustard::Detector::Definition

namespace Musae::SimFlux {

// inline namespace SD {
// class EarthSD;
// } // namespace SD

inline namespace Action {

class DetectorConstruction final : public Mustard::Env::Memory::PassiveSingleton<DetectorConstruction>,
                                   public G4VUserDetectorConstruction {
public:
    DetectorConstruction(bool checkOverlap);

    auto Construct() -> G4VPhysicalVolume* override;

public:
    using DescriptionInUse = std::tuple<Detector::Description::Hierarchy,
                                        Detector::Description::LGA,
                                        Detector::Description::Material,
                                        Detector::Description::Model,
                                        Detector::Description::Terrain,
                                        Detector::Description::World>;

private:
    bool fCheckOverlap;

    std::unique_ptr<Mustard::Detector::Definition::DefinitionBase> fWorld;
};

} // namespace Action

} // namespace Musae::SimFlux
