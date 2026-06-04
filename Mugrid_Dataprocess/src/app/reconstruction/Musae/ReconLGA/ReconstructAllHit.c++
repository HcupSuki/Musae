#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructAllHit.h++"

#include <ranges>
#include <unordered_set>

namespace Musae::ReconLGA {

auto PreprocessDigiData(
    muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
    size_t maxTimeOutliers = 1,
    size_t maxDistanceOutliers = 1) -> muc::flat_hash_map<char, std::vector<LGADigi*>> {

    for (auto& [axis, digis] : digiData) {
        // Check if enough data points for filtering
        if (digis.size() <= maxTimeOutliers + maxDistanceOutliers) {
            continue; // Too few data points, skip
        }

        // Step 1: Mark points with largest time values
        std::vector<std::pair<float, size_t>> timeValues;
        timeValues.reserve(digis.size());

        for (size_t i = 0; i < digis.size(); ++i) {
            timeValues.emplace_back(Get<"t">(*digis[i]), i);
        }

        // Sort by time (descending)
        std::sort(timeValues.begin(), timeValues.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        // Mark points with largest time values
        std::unordered_set<size_t> indicesToRemove;
        for (size_t i = 0; i < maxTimeOutliers && i < timeValues.size(); ++i) {
            indicesToRemove.insert(timeValues[i].second);
        }

        // Step 2: Find point with minimum time and its fiberLocID
        auto minTimeIter = std::min_element(timeValues.begin(), timeValues.end(),
                                            [](const auto& a, const auto& b) { return a.first < b.first; });

        size_t minTimeIndex = minTimeIter->second;
        short referenceLocID = Get<"FibLocID">(*digis[minTimeIndex]);

        // Step 3: Compute fiberLocID distance, find farthest points
        std::vector<std::pair<int, size_t>> distances;
        distances.reserve(digis.size());

        for (size_t i = 0; i < digis.size(); ++i) {
            int distance = std::abs(Get<"FibLocID">(*digis[i]) - referenceLocID);
            distances.emplace_back(distance, i);
        }

        // Sort by distance (descending)
        std::sort(distances.begin(), distances.end(),
                  [](const auto& a, const auto& b) { return a.first > b.first; });

        // Mark farthest points
        size_t distanceOutliersAdded = 0;
        for (const auto& [distance, index] : distances) {
            if (distanceOutliersAdded >= maxDistanceOutliers) break;

            // Add if not already marked for removal
            // if (indicesToRemove.insert(index).second) {
            //     distanceOutliersAdded++;
            // }
            indicesToRemove.insert(index);
            distanceOutliersAdded++;
        }

        // Step 4: Create new array excluding marked points
        std::vector<LGADigi*> filteredDigis;
        filteredDigis.reserve(digis.size() - indicesToRemove.size());

        for (size_t i = 0; i < digis.size(); ++i) {
            if (indicesToRemove.count(i) == 0) {
                filteredDigis.push_back(digis[i]);
            }
        }

        // Update result
        digis = std::move(filteredDigis);
        // // Debug: output actual number of removed points
        // std::cout << "Axis " << axis << ": Time outliers marked: " << maxTimeOutliers 
        // << ", Distance outliers marked: " << distanceOutliersAdded
        // << ", Total unique outliers: " << indicesToRemove.size() << std::endl;
    }

    return digiData;
}

auto ReconstructAllHit(const LGADigiMap<std::unique_ptr<LGADigi>>& eventDigi, ReconstructHitMethod method, long long & UpEventCount)
    -> muc::unique_ptrvec<LGAHit> {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    muc::unique_ptrvec<LGAHit> eventHit;
    eventHit.reserve(lga.NModule());
    LGADigiMap<LGADigi*> goodEventDigi;
    goodEventDigi.reserve(lga.NModule());

    // digi selection loop

    int hitID{};
    float tup{}, tdown{};
    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{eventDigi.find(moduleID)};
        if (iDigiOfTheModule == eventDigi.cend()) {
            continue;
        }
        const auto& digiOfTheModule{iDigiOfTheModule->second};

        for (auto edge : {'x', 'y'}) {
            const auto iDigiOfTheEdge{digiOfTheModule.find(edge)};
            if (iDigiOfTheEdge == digiOfTheModule.cend()) {
                break;
            }
            const auto& digiOfTheEdge{iDigiOfTheEdge->second};
            // digi selection here
            for (auto&& digi : digiOfTheEdge) {
                if (Get<"energy">(*digi) > lga.LuminousDigiEnergyThreshold()) {
                    goodEventDigi[moduleID][edge].emplace_back(digi.get());
                }
            }
        }
    }

    // hit reconstruction loop
    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{goodEventDigi.find(moduleID)};
        if (iDigiOfTheModule == goodEventDigi.cend()) {
            continue;
        }
        auto& digiOfTheModule{iDigiOfTheModule->second};

        if (not digiOfTheModule.contains('x') or
            not digiOfTheModule.contains('y')) {
            digiOfTheModule.clear();
            continue;
        }

        auto preDigiOfTheModule{PreprocessDigiData(digiOfTheModule)};
        auto hit{ReconstructHit(preDigiOfTheModule, hitID, method)};
        // auto hit{ReconstructHit(digiOfTheModule, hitID, method)};
        if (hit == nullptr) {
            digiOfTheModule.clear();
            continue;
        }
        if (Get<"ModID">(*hit) == 0) {
            tdown = Get<"t">(*hit);
        }
        else if (Get<"ModID">(*hit) == lga.NModule() - 1) {
            tup = Get<"t">(*hit);
        }

        eventHit.emplace_back(std::move(hit));
        ++hitID;    
    }

    // n hit selection
    if (ssize(eventHit) < lga.NHitPreThreshold()) {
        return {};
    }

    for (auto&& [_, digiOfTheModule] : std::as_const(goodEventDigi)) {
        for (auto&& [_, digiOfTheEdge] : digiOfTheModule) {
            for (auto&& digi : digiOfTheEdge) {
                Get<"Good">(*digi) = true;
            }
        }
    }
    if (tup != 0 and tdown != 0) {
        const auto dt{tup - tdown};
        if (dt > 0) {
            ++UpEventCount;
        }
    }
    return eventHit;
}

} // namespace Musae::ReconLGA
