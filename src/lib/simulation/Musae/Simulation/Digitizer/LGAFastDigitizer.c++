#include "Musae/Detector/Description/LGA.h++"
#include "Musae/Simulation/Digitizer/LGAFastDigitizer.h++"

#include "Mustard/Utility/PrettyLog.h++"

#include "G4DigiManager.hh"

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
                // construct digi (a real hit)
                const auto digi{new LGAFastDigi{
                    **std::ranges::min_element(
                        cluster,
                        [](const auto& hit1, const auto& hit2) {
                            return Get<"TrkID">(*hit1) < Get<"TrkID">(*hit2);
                        })}};
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
