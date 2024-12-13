#include "Musae/SimFlux/Action/SteppingAction.h++"

#include "G4Step.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"

#include "muc/math"

#include "gsl/gsl"

namespace Musae::SimFlux::inline Action {

SteppingAction::SteppingAction() :
    PassiveSingleton{},
    G4UserSteppingAction{},
    fEnableNonMuonKiller{},
    fPhysicsMessengerRegister{this} {}

auto SteppingAction::UserSteppingAction(const G4Step* step) -> void {
    if (fEnableNonMuonKiller) {
        if (auto& track{*step->GetTrack()};
            muc::abs(track.GetParticleDefinition()->GetPDGEncoding()) != 13) {
            track.SetTrackStatus(fKillTrackAndSecondaries);
            return;
        }
    }
}

} // namespace Musae::SimFlux::inline Action
