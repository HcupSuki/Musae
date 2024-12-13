#pragma once

#include "Mustard/Extension/Geant4X/Interface/SingletonMessenger.h++"

#include <memory>

class G4UIcmdWithABool;

namespace Musae::SimFlux {

inline namespace Action {
class SteppingAction;
} // namespace Action
class PhysicsList;

inline namespace Messenger {

class PhysicsMessenger final : public Mustard::Geant4X::SingletonMessenger<PhysicsMessenger,
                                                                           PhysicsList,
                                                                           SteppingAction> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    PhysicsMessenger();
    ~PhysicsMessenger();

public:
    auto SetNewValue(G4UIcommand* command, G4String value) -> void override;

private:
    std::unique_ptr<G4UIcmdWithABool> fNonMuonProcessActivation;
};

} // namespace Messenger

} // namespace Musae::SimFlux
