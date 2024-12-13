#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4UserEventAction.hh"

namespace Musae::SimFlux::inline Action {

class EventAction final : public Mustard::Env::Memory::PassiveSingleton<EventAction>,
                          public G4UserEventAction {
public:
    auto EndOfEventAction(const G4Event*) -> void override;
};

} // namespace Musae::SimFlux::inline Action
