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

#include "Mustard/Utility/PrettyLog.h++"

#include "G4DigiManager.hh"
#include "G4TwoVector.hh"

#include "muc/algorithm"
#include "muc/numeric"

#include <ranges>
#include <tuple>

namespace Musae::inline Simulation::inline Digitizer {

LGAFastDigitizer::LGAFastDigitizer() :
    LGADigitizerBase{"LGAFastDigitizer"},
    fDigiCollection{} {
    collectionName.emplace_back("LGAFastDigiCollection");
    G4DigiManager::GetDMpointer()->AddNewModule(this);
}

auto LGAFastDigitizer::Digitize() -> void {
    fDigiCollection = new LGAFastDigiCollection{"LGAFastDigitizer", collectionName[0]};
    StoreDigiCollection(fDigiCollection);

    fDigiCollection->GetVector()->reserve(
        muc::ranges::accumulate(*fHitMap, 0,
                                [](auto&& count, auto&& modHit) {
                                    return count + modHit.second.size();
                                }));

    for (auto&& [modID, hitVector] : *fHitMap) {
        switch (hitVector.size()) {
        case 0:
            break;
        case 1: {
            const auto& hit{*hitVector.front()};
            assert(Get<"ModID">(hit) == modID);
            fDigiCollection->insert(new LGAFastDigi{hit});
        } break;
        default: {
            const auto timeResolutionFWHM{Detector::Description::LGA::Instance().TimeResolutionFWHM()};
            assert(timeResolutionFWHM >= 0);
            // loop over all hits and cluster to real hits by times
            std::ranges::subrange cluster{hitVector.begin(), hitVector.begin()};
            while (cluster.end() != hitVector.end()) {
                const auto tFirst{*Get<"t">(**cluster.end())};
                const auto windowClosingTime{tFirst + timeResolutionFWHM};
                if (tFirst == windowClosingTime and // Notice: bad numeric with huge Get<"t">(**clusterFirst)!
                    timeResolutionFWHM != 0) [[unlikely]] {
                    Mustard::PrettyWarning(fmt::format("A huge time ({}) completely rounds off the time resolution ({})", tFirst, timeResolutionFWHM));
                }
                cluster = {cluster.end(), std::ranges::find_if_not(
                                              cluster.end(), hitVector.end(),
                                              [&windowClosingTime](const auto& hit) {
                                                  return Get<"t">(*hit) <= windowClosingTime;
                                              })};
                const auto& firstHit = **std::ranges::min_element(
                    cluster, [](const auto& hit1, const auto& hit2) {
                        return Get<"t">(*hit1) < Get<"t">(*hit2);
                    });
                const auto& lastHit = **std::ranges::max_element(
                    cluster, [](const auto& hit1, const auto& hit2) {
                        return Get<"t">(*hit1) < Get<"t">(*hit2);
                    });
                // construct digi (a real hit)
                const auto digi{new LGAFastDigi{
                    **std::ranges::min_element(
                        cluster,
                        [](const auto& hit1, const auto& hit2) {
                            return Get<"TrkID">(*hit1) < Get<"TrkID">(*hit2);
                        })}};
                auto firstHitPos = Get<"x">(firstHit);
                auto lastHitPos = Get<"x">(lastHit);
                const G4TwoVector avgPosition{(firstHitPos[0] + lastHitPos[0]) / 2,
                                              (firstHitPos[1] + lastHitPos[1]) / 2};
                Get<"x">(*digi) = avgPosition;
                Get<"Edep">(*digi) = 0;
                for (const auto& hit : cluster) {
                    assert(Get<"ModID">(*hit) == modID);
                    Get<"Edep">(*digi) += Get<"Edep">(*hit);
                }
                fDigiCollection->insert(digi);
            }
        } break;
        }
    }

    muc::timsort(*fDigiCollection->GetVector(),
                 [](const auto& digi1, const auto& digi2) {
                     return std::tie(Get<"TrkID">(*digi1), Get<"t">(*digi1)) <
                            std::tie(Get<"TrkID">(*digi2), Get<"t">(*digi2));
                 });

    for (int hitID{}; auto&& digi : *fDigiCollection->GetVector()) {
        Get<"HitID">(*digi) = hitID++;
    }
}

} // namespace Musae::inline Simulation::inline Digitizer
