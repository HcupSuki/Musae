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

#include "Musae/Simulation/Digi/LGAFastDigi.h++"
#include "Musae/Simulation/Hit/LGAHit.h++"

#include "Mustard/Utility/NonMoveableBase.h++"

#include "G4VDigitizerModule.hh"

#include "muc/hash_map"

#include <string>
#include <vector>

namespace Musae::inline Simulation::inline Digitizer {

class LGADigitizerBase : public Mustard::NonMoveableBase,
                         public G4VDigitizerModule {
public:
    LGADigitizerBase(std::string name);
    virtual ~LGADigitizerBase() = default;

    auto HitMap(const auto& hc) -> void { fHitMap = &hc; }

protected:
    const muc::flat_hash_map<int, std::vector<LGAHit*>>* fHitMap;
};

} // namespace Musae::inline Simulation::inline Digitizer
