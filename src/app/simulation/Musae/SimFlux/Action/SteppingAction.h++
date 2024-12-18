#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4UserSteppingAction.hh"

#include <any>

namespace Musae::SimFlux::inline Action {

class SteppingAction final : public Mustard::Env::Memory::PassiveSingleton<SteppingAction>,
                             public G4UserSteppingAction {
public:
    SteppingAction();
    ~SteppingAction();

    auto EnableNonMuonKiller(bool val) -> void { fEnableNonMuonKiller = val; }

    auto UserSteppingAction(const G4Step* step) -> void override;

private:
    bool fEnableNonMuonKiller;

    std::any fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux::inline Action
