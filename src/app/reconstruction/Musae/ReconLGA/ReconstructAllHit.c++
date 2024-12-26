#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructAllHit.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"

#include <ranges>

namespace Musae::ReconLGA {

auto ReconstructAllHit(const LGADigiMap<std::unique_ptr<LGADigi>>& coincidentDigi,
                       int eventID, std::string_view method)
    -> std::pair<std::vector<std::unique_ptr<LGAHit>>, LGADigiMap<LGADigi*>> {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    std::pair<std::vector<std::unique_ptr<LGAHit>>, LGADigiMap<LGADigi*>> result;
    auto& [eventHit, eventDigi]{result};
    eventHit.reserve(lga.NModule());
    eventDigi.reserve(lga.NModule());

    // digi selection loop

    int hitID{};
    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{coincidentDigi.find(moduleID)};
        if (iDigiOfTheModule == coincidentDigi.cend()) {
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
                    eventDigi[moduleID][edge].emplace_back(digi.get());
                }
            }
        }
    }

    // hit reconstruction loop

    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{eventDigi.find(moduleID)};
        if (iDigiOfTheModule == eventDigi.cend()) {
            continue;
        }
        auto& digiOfTheModule{iDigiOfTheModule->second};

        if (not digiOfTheModule.contains('x') or
            not digiOfTheModule.contains('y')) {
            digiOfTheModule.clear();
            continue;
        }

        auto hit{ReconstructHit(digiOfTheModule, eventID, hitID, method)};
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

    return result;
}

} // namespace Musae::ReconLGA
