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
