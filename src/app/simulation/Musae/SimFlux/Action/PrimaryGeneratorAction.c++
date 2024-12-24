#include "Musae/SimFlux/Action/PrimaryGeneratorAction.h++"

namespace Musae::SimFlux::inline Action {

PrimaryGeneratorAction::PrimaryGeneratorAction() :
    PassiveSingleton{this} {}

auto PrimaryGeneratorAction::GeneratePrimaries(G4Event* event) -> void {
    fGenerator.GeneratePrimaryVertex(event);
}

} // namespace Musae::SimFlux
