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

#include "Musae/Data/SimHit.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Extension/Geant4X/Memory/UseG4Allocator.h++"

#include "G4TDigiCollection.hh"
#include "G4VDigi.hh"

namespace Musae::inline Simulation::inline Digi {

class LGAFastDigi final : public Mustard::Geant4X::UseG4Allocator<LGAFastDigi>,
                          public G4VDigi,
                          public Mustard::Data::Tuple<Data::LGASimHit> {
public:
    LGAFastDigi() = default;
    ~LGAFastDigi() = default;
    inline LGAFastDigi(const Tuple& t);
    inline LGAFastDigi(Tuple&& t);
};

using LGAFastDigiCollection = G4TDigiCollection<LGAFastDigi>;

} // namespace Musae::inline Simulation::inline Digi

#include "Musae/Simulation/Digi/LGAFastDigi.inl"
