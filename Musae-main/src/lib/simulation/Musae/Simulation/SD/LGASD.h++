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

#include "Musae/Simulation/Hit/LGAHit.h++"

#include "Mustard/Utility/NonMoveableBase.h++"

#include "G4VSDFilter.hh"
#include "G4VSensitiveDetector.hh"

#include <memory>
#include <unordered_map>
#include <vector>

namespace Musae::inline Simulation {

inline namespace Digitizer {
class LGADigitizerBase;
} // namespace Digitizer

inline namespace SD {

class LGASD : public Mustard::NonMoveableBase,
              public G4VSensitiveDetector {
public:
    LGASD();
    ~LGASD();

    virtual auto Initialize(G4HCofThisEvent* hitsCollection) -> void override;
    virtual auto ProcessHits(G4Step* theStep, G4TouchableHistory*) -> bool override;
    virtual auto EndOfEvent(G4HCofThisEvent*) -> void override;

protected:
    class LGASDFilter : public Mustard::NonMoveableBase,
                        public G4VSDFilter {
        using G4VSDFilter::G4VSDFilter;
        virtual auto Accept(const G4Step* step) const -> bool;
    };

protected:
    LGAHitsCollection* fHitsCollection;
    muc::flat_hash_map<int, std::vector<LGAHit*>> fHitMap;

    LGASDFilter fFilter;
    std::unique_ptr<LGADigitizerBase> fDigitizer;
};

} // namespace SD

} // namespace Musae::inline Simulation
