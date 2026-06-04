#include "Musae/SimFlux/Action/SteppingAction.h++"
#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"
#include "Musae/SimFlux/Analysis.h++"

#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4Track.hh"
#include "G4TrackStatus.hh"

#include "G4Event.hh"
#include "G4EventManager.hh"

#include "G4ios.hh"

#include "muc/math"

#include "gsl/gsl"

namespace Musae::SimFlux::inline Action {

namespace {
constexpr bool kSteppingGeometryDebug{false};
}

SteppingAction::SteppingAction() :
    PassiveSingleton{this},
    G4UserSteppingAction{},
    fEnableNonMuonKiller{},
    fPerfectRangeGeneration{},
    fPhysicsMessengerRegister{std::in_place_type<PhysicsMessenger::Register<SteppingAction>>, this} {}

SteppingAction::~SteppingAction() = default;

auto SteppingAction::UserSteppingAction(const G4Step* step) -> void {
    if (kSteppingGeometryDebug) {
        const auto* preVolume{step->GetPreStepPoint()->GetPhysicalVolume()};
        const auto* postVolume{step->GetPostStepPoint()->GetPhysicalVolume()};
        const auto preName{preVolume ? preVolume->GetName() : "NULL"};
        const auto postName{postVolume ? postVolume->GetName() : "NULL"};
        const auto shouldTrace{preName == "LGAScintillator" || postName == "LGAScintillator" ||
                               preName.find("Model_") != std::string::npos || postName.find("Model_") != std::string::npos};

        if (shouldTrace) {
             const auto* currentEvent{G4EventManager::GetEventManager()->GetConstCurrentEvent()};
            G4cout << "[SteppingDebug] evt=" << (currentEvent ? currentEvent->GetEventID() : -1)
                   << " trk=" << step->GetTrack()->GetTrackID()
                   << " pdg=" << step->GetTrack()->GetParticleDefinition()->GetPDGEncoding()
                   << " edep=" << step->GetTotalEnergyDeposit()
                   << " stepLen=" << step->GetStepLength()
                   << " preVol=" << preName
                   << " postVol=" << postName
                   << " stepStatus=" << step->GetPostStepPoint()->GetStepStatus()
                   << " prePos=" << step->GetPreStepPoint()->GetPosition()
                   << " postPos=" << step->GetPostStepPoint()->GetPosition()
                   << G4endl;
        }
    }

    if (fEnableNonMuonKiller) {
        if (auto& track{*step->GetTrack()};
            muc::abs(track.GetParticleDefinition()->GetPDGEncoding()) != 13) {
            track.SetTrackStatus(fKillTrackAndSecondaries);
            return;
        }
    }
    if (fPerfectRangeGeneration) {
        auto& track{*step->GetTrack()};
        auto Hit{Analysis::Instance().GetHit()};
        if (muc::abs(track.GetParticleDefinition()->GetPDGEncoding()) != 0) {
            track.SetTrackStatus(fKillTrackAndSecondaries);
            // return;
        }
        else if (!Hit){
            G4double density = step->GetPreStepPoint()->GetMaterial()->GetDensity();
            G4double stepLength = step->GetStepLength();
            Analysis::Instance().AddRange(density * stepLength);// unit: MeV/mm^3 * mm = MeV/mm^2
        }
        if (track.GetVolume()->GetName() == "LGAScintillator") {
            Analysis::Instance().SetHit(true);
        }
    }
}

} // namespace Musae::SimFlux::inline Action
