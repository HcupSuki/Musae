#pragma once

#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4UserSteppingAction.hh"

namespace Musae::SimFlux::inline Action {

class SteppingAction final : public Mustard::Env::Memory::PassiveSingleton<SteppingAction>,
                             public G4UserSteppingAction {
public:
    SteppingAction();

    auto EnableNonMuonKiller(bool val) -> void { fEnableNonMuonKiller = val; }

    auto UserSteppingAction(const G4Step* step) -> void override;

private:
    bool fEnableNonMuonKiller;

    PhysicsMessenger::Register<SteppingAction> fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux::inline Action
