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
#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"

#include "G4UIcmdWithABool.hh"


namespace Musae::SimFlux::inline Messenger {

PhysicsMessenger::PhysicsMessenger() :
    SingletonMessenger{},
    fNonMuonProcessActivation{},
    fPerfectRangeGeneration{}{
    fNonMuonProcessActivation = std::make_unique<G4UIcmdWithABool>("/Musae/Physics/NonMuonProcessActivation", this);
    fNonMuonProcessActivation->SetParameterName("active", false);
    fNonMuonProcessActivation->SetGuidance("Set whether to activate processes for non-muon particles.");
    fNonMuonProcessActivation->AvailableForStates(G4State_Idle);
    fPerfectRangeGeneration = std::make_unique<G4UIcmdWithABool>("/Musae/Physics/PerfectRangeGeneration", this);
    fPerfectRangeGeneration->SetParameterName("active", false);
    fPerfectRangeGeneration->SetGuidance("Set whether to activate generate perfect range data for muons.");
    fPerfectRangeGeneration->AvailableForStates(G4State_Idle);
}

PhysicsMessenger::~PhysicsMessenger() = default;

auto PhysicsMessenger::SetNewValue(G4UIcommand* command, G4String value) -> void {
    if (command == fNonMuonProcessActivation.get()) {
        const auto active{fNonMuonProcessActivation->GetNewBoolValue(value)};
        Deliver<PhysicsList>([&](auto&& r) {
            r.NonMuonProcessActivation(active);
        });
        Deliver<SteppingAction>([&](auto&& r) {
            r.EnableNonMuonKiller(not active);
        });
    }
    if (command == fPerfectRangeGeneration.get()) {
        const auto active{fPerfectRangeGeneration->GetNewBoolValue(value)};
        if (!active){
            // std::cout << "Perfect range generation is not implemented yet." << std::endl;
            return;
        }
        Deliver<PhysicsList>([&](auto&& r) {
            r.NonGeantinoProcessDeactivation(active);
        });
        Deliver<SteppingAction>([&](auto&& r) {
            r.EnablePerfectRangeGeneration(active);
        });
        Deliver<PrimaryGeneratorAction>([&](auto&& r) {
            r.EnablePerfectRangeGeneration(active);
        });
    }
}

} // namespace Musae::SimFlux::inline Messenger
