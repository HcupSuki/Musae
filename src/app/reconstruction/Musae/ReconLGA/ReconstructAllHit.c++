#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructAllHit.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"

#include <ranges>

namespace Musae::ReconLGA {

auto ReconstructAllHit(const LGADigiMap<std::unique_ptr<LGADigi>>& eventDigi, std::string_view method)
    -> std::vector<std::unique_ptr<LGAHit>> {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    std::vector<std::unique_ptr<LGAHit>> eventHit;
    eventHit.reserve(lga.NModule());
    LGADigiMap<LGADigi*> goodEventDigi;
    goodEventDigi.reserve(lga.NModule());

    // digi selection loop

    int hitID{};
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

        auto hit{ReconstructHit(digiOfTheModule, hitID, method)};
        if (hit == nullptr) {
            digiOfTheModule.clear();
            continue;
        }

        eventHit.emplace_back(std::move(hit));
        ++hitID;
    }

    // n hit selection
    if (ssize(eventHit) < lga.NHitThreshold()) {
        return {};
    }

    for (auto&& [_, digiOfTheModule] : std::as_const(goodEventDigi)) {
        for (auto&& [_, digiOfTheEdge] : digiOfTheModule) {
            for (auto&& digi : digiOfTheEdge) {
                Get<"Good">(*digi) = true;
            }
        }
    }

    return eventHit;
}

} // namespace Musae::ReconLGA
