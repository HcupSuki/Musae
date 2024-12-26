#pragma once

#include "Musae/ReconLGA/Type.h++"

#include <memory>
#include <vector>

namespace Musae::ReconLGA {

// plot hit of an event
auto PlotEvent(const LGADigiMap<std::unique_ptr<LGADigi>>& coincidentDigi,
               const std::vector<std::unique_ptr<LGAHit>>& eventHit,
               const LGADigiMap<LGADigi*>& eventDigi,
               const CRMuEvent* crMuEvent) -> void;

} // namespace Musae::ReconLGA
