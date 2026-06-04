#include "Musae/SimFlux/Action/EventAction.h++"
#include "Musae/SimFlux/Analysis.h++"

namespace Musae::SimFlux::inline Action {

EventAction::EventAction() :
    PassiveSingleton{this} {}

auto EventAction::EndOfEventAction(const G4Event*) -> void {
    Analysis::Instance().EventEndAction();
}

} // namespace Musae::SimFlux::inline Action
