#pragma once

#include "Musae/ReconLGA/ReconstructHit.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/ptrvec"

#include <memory>
#include <utility>
#include <vector>

namespace Musae::ReconLGA {

// coincident digi -> {event hit, good digi}
auto ReconstructAllHit(const LGADigiMap<std::unique_ptr<LGADigi>>& coincidentDigi, ReconstructHitMethod method, long long & UpEventCount)
    -> muc::unique_ptrvec<LGAHit>;

} // namespace Musae::ReconLGA
