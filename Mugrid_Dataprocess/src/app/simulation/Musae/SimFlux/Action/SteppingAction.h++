#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4UserSteppingAction.hh"
#include "G4Types.hh"

#include <any>

namespace Musae::SimFlux::inline Action {

class SteppingAction final : public Mustard::Env::Memory::PassiveSingleton<SteppingAction>,
                             public G4UserSteppingAction {
public:
    SteppingAction();
    ~SteppingAction();

    auto EnableNonMuonKiller(bool val) -> void { fEnableNonMuonKiller = val; }
    auto EnablePerfectRangeGeneration(bool val) -> void { fPerfectRangeGeneration = val; }

    auto UserSteppingAction(const G4Step* step) -> void override;

private:
    bool fEnableNonMuonKiller;
    bool fPerfectRangeGeneration;


    std::any fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux::inline Action
