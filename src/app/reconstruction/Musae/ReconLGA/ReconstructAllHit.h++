#pragma once

#include "Musae/ReconLGA/Type.h++"

#include <memory>
#include <string_view>
#include <utility>
#include <vector>

namespace Musae::ReconLGA {

// coincident digi -> {event hit, good digi}
auto ReconstructAllHit(const LGADigiMap<std::unique_ptr<LGADigi>>& coincidentDigi, std::string_view method)
    -> std::vector<std::unique_ptr<LGAHit>>;

} // namespace Musae::ReconLGA
