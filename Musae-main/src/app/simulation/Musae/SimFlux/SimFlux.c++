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
#include "Musae/SimFlux/DefaultMacro.h++"
#include "Musae/SimFlux/RunManager.h++"
#include "Musae/SimFlux/SimFlux.h++"

#include "Mustard/Env/CLI/Geant4CLI.h++"
#include "Mustard/Env/MPIEnv.h++"
#include "Mustard/Extension/Geant4X/Interface/MPIExecutive.h++"
#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/UseXoshiro.h++"

#include "G4GeometryManager.hh"

#include <algorithm>


namespace Musae::SimFlux {

SimFlux::SimFlux() :
    Subprogram{"SimFlux", "Simulation of cosmic ray muon flux in underground experiment."} {}

auto SimFlux::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::Geant4CLI<> cli;
    Mustard::Env::MPIEnv env{argc, argv, cli};

    Mustard::UseXoshiro<256> random;
    cli.SeedRandomIfFlagged();

    // Set geometry tolerance
    using namespace Mustard::LiteralUnit::Length;
    G4GeometryManager::GetInstance()->SetWorldMaximumExtent(10_km); // just a scale
    // PhysicsList, DetectorConstruction, ActionInitialization are instantiated in RunManager constructor.
    // Mutually exclusive random seeds are distributed to all processes upon each BeamOn.
    Musae::SimFlux::RunManager runManager;
    Mustard::Geant4X::MPIExecutive{}.StartSession(cli, Musae::SimFlux::defaultMacro);

    return EXIT_SUCCESS;
}

} // namespace Musae::SimFlux
