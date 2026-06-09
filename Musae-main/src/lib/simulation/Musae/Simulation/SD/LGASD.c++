// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/Simulation/Digitizer/LGAFastDigitizer.h++"
#include "Musae/Simulation/SD/LGASD.h++"

#include "G4Event.hh"
#include "G4EventManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4ios.hh"
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

namespace {
constexpr bool kLGASDDebug{false};
}

LGASD::LGASD() :
    NonMoveableBase{},
    G4VSensitiveDetector{Detector::Description::LGA::Instance().Name()},
    fHitsCollection{},
    fHitMap{},
    fFilter{SensitiveDetectorName + "Filter"},
    fDigitizer{std::make_unique<LGAFastDigitizer>()} {
    SetFilter(&fFilter);
    collectionName.insert(SensitiveDetectorName + "HC");

    if (kLGASDDebug) {
        G4cout << "[LGASD] Constructed SD: " << SensitiveDetectorName << G4endl;
    }
}

LGASD::~LGASD() = default;

auto LGASD::Initialize(G4HCofThisEvent* hitsCollectionOfThisEvent) -> void {
    fHitsCollection = new LGAHitsCollection{SensitiveDetectorName, collectionName[0]};
    const auto hcID{G4SDManager::GetSDMpointer()->GetCollectionID(fHitsCollection)};
    hitsCollectionOfThisEvent->AddHitsCollection(hcID, fHitsCollection);

    if (kLGASDDebug) {
        G4cout << "[LGASD] Initialize event collection, HCID = " << hcID
               << ", collection = " << collectionName[0] << G4endl;
    }
}

auto LGASD::ProcessHits(G4Step* theStep, G4TouchableHistory*) -> G4bool {
    const auto& step{*theStep};
    const auto& track{*step.GetTrack()};
    const auto& particle{*track.GetDefinition()};
    const auto& preStepPoint{*step.GetPreStepPoint()};
    const auto& postStepPoint{*step.GetPostStepPoint()};
    const auto& touchable{*preStepPoint.GetTouchable()};
    const auto modID{touchable.GetReplicaNumber(0)};
    const auto detID{touchable.GetReplicaNumber(1)};

        if (kLGASDDebug) {
         const auto* preVolume{preStepPoint.GetPhysicalVolume()};
         const auto* postVolume{postStepPoint.GetPhysicalVolume()};
         G4cout << "[LGASD] ProcessHits"
             << " evt=" << G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID()
             << " trk=" << track.GetTrackID()
             << " pdg=" << particle.GetPDGEncoding()
             << " edep=" << step.GetTotalEnergyDeposit()
             << " detID=" << detID
             << " modID=" << modID
             << " preVol=" << (preVolume ? preVolume->GetName() : "NULL")
             << " postVol=" << (postVolume ? postVolume->GetName() : "NULL")
             << " preCopy=" << (preVolume ? preVolume->GetCopyNo() : -1)
             << " postStatus=" << postStepPoint.GetStepStatus()
             << G4endl;

         G4cout << "[LGASD]   posPre=" << preStepPoint.GetPosition()
             << " posPost=" << postStepPoint.GetPosition()
             << " localXY=" << *touchable.GetRotation() * ((preStepPoint.GetPosition() + postStepPoint.GetPosition()) / 2 - touchable.GetTranslation())
             << G4endl;
        }

    // transform hit position to local coordinate
    const G4TwoVector averageHitPosition{*touchable.GetRotation() * ((preStepPoint.GetPosition() + postStepPoint.GetPosition())/2 - touchable.GetTranslation())};
    // new a hit
    const auto hit{new LGAHit};
    Get<"EvtID">(*hit) = G4EventManager::GetEventManager()->GetConstCurrentEvent()->GetEventID();
    Get<"DetID">(*hit) = detID;
    Get<"HitID">(*hit) = -1; // to be determined
    Get<"ModID">(*hit) = modID;
    Get<"Edep">(*hit) = step.GetTotalEnergyDeposit();
    Get<"t">(*hit) = preStepPoint.GetGlobalTime();
    Get<"x">(*hit) = averageHitPosition;
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
    if (kLGASDDebug) {
        std::size_t hitCount{};
        for (const auto& [modID, hitVector] : fHitMap) {
            hitCount += hitVector.size();
            G4cout << "[LGASD] EndOfEvent module=" << modID
                   << " hits=" << hitVector.size() << G4endl;
        }
        G4cout << "[LGASD] EndOfEvent total hits = " << hitCount << G4endl;
    }

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
    const auto threshold{Detector::Description::LGA::Instance().EnergyDepositionThreshold()};
    const auto edep{step->GetTotalEnergyDeposit()};
    const auto accepted{edep > threshold};

    if (kLGASDDebug) {
        const auto* preStepPoint{step->GetPreStepPoint()};
        const auto* volume{preStepPoint ? preStepPoint->GetPhysicalVolume() : nullptr};
        G4cout << "[LGASD] Filter"
               << " edep=" << edep
               << " threshold=" << threshold
               << " accepted=" << accepted
               << " volume=" << (volume ? volume->GetName() : "NULL")
               << G4endl;
    }

    return accepted;
}

} // namespace Musae::inline Simulation::inline SD
