#include "Musae/SimFlux/Action/PrimaryGeneratorAction.h++"
#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"

#include "G4Event.hh"
#include "G4Geantino.hh"
#include "G4ParticleTable.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"

namespace Musae::SimFlux::inline Action {

PrimaryGeneratorAction::PrimaryGeneratorAction() :
    PassiveSingleton{this},
    fPerfectRangeGeneration{},
    fPhysicsMessengerRegister{std::in_place_type<PhysicsMessenger::Register<PrimaryGeneratorAction>>, this} {}

auto PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) -> void {
    fGenerator.GeneratePrimaryVertex(event);
    if (fPerfectRangeGeneration) {
        event->GetPrimaryVertex()->GetPrimary()->SetParticleDefinition(G4Geantino::Definition());
    }
}

} // namespace Musae::SimFlux::inline Action
