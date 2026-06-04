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
