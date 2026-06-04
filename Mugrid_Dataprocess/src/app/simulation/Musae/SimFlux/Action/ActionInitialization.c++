#include "Musae/SimFlux/Action/ActionInitialization.h++"
#include "Musae/SimFlux/Action/EventAction.h++"
#include "Musae/SimFlux/Action/PrimaryGeneratorAction.h++"
#include "Musae/SimFlux/Action/RunAction.h++"
#include "Musae/SimFlux/Action/SteppingAction.h++"

namespace Musae::SimFlux::inline Action {

ActionInitialization::ActionInitialization() :
    PassiveSingleton{this} {}

auto ActionInitialization::Build() const -> void {
    SetUserAction(new RunAction);
    SetUserAction(new EventAction);
    SetUserAction(new PrimaryGeneratorAction);
    SetUserAction(new SteppingAction);
}

} // namespace Musae::SimFlux::inline Action
