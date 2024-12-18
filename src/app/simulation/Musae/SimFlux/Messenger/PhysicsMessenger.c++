#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"

#include "G4UIcmdWithABool.hh"

namespace Musae::SimFlux::inline Messenger {

PhysicsMessenger::PhysicsMessenger() :
    SingletonMessenger{},
    fNonMuonProcessActivation{} {
    fNonMuonProcessActivation = std::make_unique<G4UIcmdWithABool>("/Musae/Physics/NonMuonProcessActivation", this);
    fNonMuonProcessActivation->SetParameterName("active", false);
    fNonMuonProcessActivation->SetGuidance("Set whether to activate processes for non-muon particles.");
    fNonMuonProcessActivation->AvailableForStates(G4State_Idle);
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
}

} // namespace Musae::SimFlux::inline Messenger
