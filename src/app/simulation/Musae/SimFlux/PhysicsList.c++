#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"
#include "Musae/SimFlux/PhysicsList.h++"

#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Utility/LiteralUnit.h++"

#include "G4EmStandardPhysics_option1.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4ProcessManager.hh"
#include "G4RunManager.hh"

#include "muc/utility"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Musae::SimFlux {

using namespace Mustard::LiteralUnit::Length;

PhysicsList::PhysicsList() :
    PassiveSingleton{},
    FTFP_BERT{std::max({}, muc::to_underlying(Mustard::Env::BasicEnv::Instance().VerboseLevel()))},
    fPhysicsMessengerRegister{std::in_place_type<PhysicsMessenger::Register<PhysicsList>>, this} {
    SetDefaultCutValue(30_cm);
    ReplacePhysics(new G4EmStandardPhysics_option1{verboseLevel});
}

PhysicsList::~PhysicsList() = default;

auto PhysicsList::NonMuonProcessActivation(bool active) -> void {
    const auto particleTable{G4ParticleTable::GetParticleTable()};
    for (int i{}; i < particleTable->entries(); ++i) {
        const auto particle{particleTable->GetParticle(i)};
        if (std::abs(particle->GetPDGEncoding()) == 13) { continue; }
        const auto processManager{particle->GetProcessManager()};
        for (int j{}; j < processManager->GetProcessListLength(); ++j) {
            processManager->SetProcessActivation(j, active);
        }
    }
    if (active) {
        SetDefaultCutValue(30_cm);
    } else {
        SetDefaultCutValue(20000_km);
    }
    SetCuts();
    G4RunManager::GetRunManager()->PhysicsHasBeenModified();
}

} // namespace Musae::SimFlux
