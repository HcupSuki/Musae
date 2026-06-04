#include "Musae/SimFlux/Action/RunAction.h++"
#include "Musae/SimFlux/Analysis.h++"

#include "G4Run.hh"

namespace Musae::SimFlux::inline Action {

RunAction::RunAction() :
    PassiveSingleton{this} {}

auto RunAction::BeginOfRunAction(const G4Run* run) -> void {
    Analysis::Instance().RunBeginAction(run->GetRunID());
}

auto RunAction::EndOfRunAction(const G4Run* run) -> void {
    Analysis::Instance().RunEndAction(run->GetRunID());
}

} // namespace Musae::SimFlux::inline Action
