
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/FormatChannelSummary.h++"

#include "fmt/core.h"

#include <limits>
#include <ranges>
#include <utility>

namespace Musae::ReconLGA {

auto FormatChannelSummary(const muc::flat_hash_map<int, ChannelSummary>& flatChannelSummary) -> std::string {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};
    muc::flat_hash_map<int, muc::flat_hash_map<std::pair<char, int>, ChannelSummary>> channelSummaryData;
    for (auto&& [channelID, channelSummary] : std::as_const(flatChannelSummary)) {
        const auto ch{lga.TryChannelInfo(channelID)};
        if (ch) {
            channelSummaryData[ch->moduleID][{ch->edge, ch->fiberLocalID}] = channelSummary;
        } else {
            channelSummaryData[std::numeric_limits<int>::max()][{}] = channelSummary;
        }
    }
    std::string summaryText{"Data summary:\n"};
    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        summaryText += fmt::format("Module {} (chip {}):\n", moduleID, lga.ChipID(moduleID));
        summaryText += "  Edge x:\n";
        for (auto fiberLocalID : std::views::iota(0, lga.NFiberX())) {
            const auto& ch{channelSummaryData[moduleID][{'x', fiberLocalID}]};
            summaryText += fmt::format("    {:32} mean energy: {:0<7.6}, trigger count: {}\n",
                                       fmt::format("SiPM/Fiber {} (channel {}):", fiberLocalID, ch.channelID),
                                       ch.meanEnergy, ch.triggerCount);
        }
        summaryText += "  Edge y:\n";
        for (auto fiberLocalID : std::views::iota(0, lga.NFiberY())) {
            const auto& ch{channelSummaryData[moduleID][{'y', fiberLocalID}]};
            summaryText += fmt::format("    {:32} mean energy: {:0<7.6}, trigger count: {}\n",
                                       fmt::format("SiPM/Fiber {} (channel {}):", fiberLocalID, ch.channelID),
                                       ch.meanEnergy, ch.triggerCount);
        }
    }
    if (channelSummaryData.contains(std::numeric_limits<int>::max())) {
        summaryText += "Unknown:\n";
        for (auto&& [_, ch] : channelSummaryData.at(std::numeric_limits<int>::max())) {
            summaryText += fmt::format("  {:14} mean energy: {:0<7.6}, trigger count: {}\n",
                                       fmt::format("Channel {}:", ch.channelID),
                                       ch.meanEnergy, ch.triggerCount);
        }
    }
    return summaryText;
}

} // namespace Musae::ReconLGA
