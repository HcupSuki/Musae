#pragma once

#include "muc/hash_map"

#include <string>

namespace Musae::ReconLGA {

struct ChannelSummary {
    int channelID;
    double meanEnergy;
    unsigned triggerCount;
};

auto FormatChannelSummary(double t0, double daqTime, const muc::flat_hash_map<int, ChannelSummary>& flatChannelSummary) -> std::string;

} // namespace Musae::ReconLGA
