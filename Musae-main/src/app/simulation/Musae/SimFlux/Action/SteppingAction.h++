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

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4UserSteppingAction.hh"
#include "G4Types.hh"

#include <any>

namespace Musae::SimFlux::inline Action {

class SteppingAction final : public Mustard::Env::Memory::PassiveSingleton<SteppingAction>,
                             public G4UserSteppingAction {
public:
    SteppingAction();
    ~SteppingAction();

    auto EnableNonMuonKiller(bool val) -> void { fEnableNonMuonKiller = val; }
    auto EnablePerfectRangeGeneration(bool val) -> void { fPerfectRangeGeneration = val; }

    auto UserSteppingAction(const G4Step* step) -> void override;

private:
    bool fEnableNonMuonKiller;
    bool fPerfectRangeGeneration;


    std::any fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux::inline Action
