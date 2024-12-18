#pragma once

#include "Musae/SimFlux/Action/SteppingAction.h++"
#include "Musae/SimFlux/PhysicsList.h++"

#include "Mustard/Extension/Geant4X/Interface/SingletonMessenger.h++"

#include <memory>

class G4UIcmdWithABool;

namespace Musae::SimFlux::inline Messenger {

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

} // namespace Musae::SimFlux::inline Messenger
