#include "Musae/Detector/Description/LGA.h++"
#include "Musae/Simulation/Digitizer/LGAFastDigitizer.h++"
#include "Musae/Simulation/SD/LGASD.h++"

#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4ParticleDefinition.hh"
#include "G4RotationMatrix.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4StepPoint.hh"
#include "G4ThreeVector.hh"
#include "G4TwoVector.hh"
#include "G4VTouchable.hh"

#include "muc/algorithm"

namespace Musae::inline Simulation::inline SD {

LGASD::LGASD() :
    NonMoveableBase{},
    G4VSensitiveDetector{Detector::Description::LGA::Instance().Name()},
    fHitsCollection{},
    fHitMap{},
    fFilter{SensitiveDetectorName + "Filter"},
    fDigitizer{std::make_unique<LGAFastDigitizer>()} {
    SetFilter(&fFilter);
    collectionName.insert(SensitiveDetectorName + "HC");
}

LGASD::~LGASD() = default;

auto LGASD::Initialize(G4HCofThisEvent* hitsCollectionOfThisEvent) -> void {
    fHitsCollection = new LGAHitsCollection{SensitiveDetectorName, collectionName[0]};
    const auto hcID{G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection)};
    hitsCollectionOfThisEvent->AddHitsCollection(hcID, fHitsCollection);
}

auto LGASD::ProcessHits(G4Step* theStep, G4TouchableHistory*) -> G4bool {
    const auto& step{*theStep};
    const auto& track{*step.GetTrack()};
    const auto& particle{*track.GetDefinition()};
    const auto& preStepPoint{*step.GetPreStepPoint()};
    const auto& touchable{*preStepPoint.GetTouchable()};
    const auto modID{touchable.GetReplicaNumber()};
    // transform hit position to local coordinate
    const G4TwoVector hitPosition{*touchable.GetRotation() * (preStepPoint.GetPosition() - touchable.GetTranslation())};
    // new a hit
    const auto hit{new LGAHit};
    Get<"EvtID">(*hit) = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    Get<"HitID">(*hit) = -1; // to be determined
    Get<"ModID">(*hit) = modID;
    Get<"t">(*hit) = preStepPoint.GetGlobalTime();
    Get<"x">(*hit) = hitPosition;
    Get<"Edep">(*hit) = step.GetTotalEnergyDeposit();
    Get<"TrkID">(*hit) = track.GetTrackID();
    Get<"PDGID">(*hit) = particle.GetPDGEncoding();
    Get<"Ek">(*hit) = preStepPoint.GetKineticEnergy();
    Get<"p">(*hit) = preStepPoint.GetMomentum();
    Get<"w">(*hit) = track.GetWeight();
    fHitsCollection->insert(hit);
    fHitMap[modID].emplace_back(hit);
    return true;
}

auto LGASD::EndOfEvent(G4HCofThisEvent*) -> void {
    for (auto&& [_, hitVector] : fHitMap) {
        muc::timsort(hitVector,
                     [](const auto& hit1, const auto& hit2) {
                         return Get<"t">(*hit1) < Get<"t">(*hit2);
                     });
    }
    fDigitizer->HitMap(fHitMap);
    fDigitizer->Digitize();
    fHitMap.clear();
}

auto LGASD::LGASDFilter::Accept(const G4Step* step) const -> bool {
    return step->GetTotalEnergyDeposit() > Detector::Description::LGA::Instance().EnergyDepositionThreshold();
}

} // namespace Musae::inline Simulation::inline SD
