#pragma once

#include "Musae/Data/Event.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/ptrvec"

#include "TH2D.h"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

auto PlaneEfficiencyCal(const muc::unique_ptrvec<LGAHit>& eventHit, TH2D& hitatleast2plane, TH2D& hitAllplane, int planeid, double xRange, double yRange) -> void;

} // namespace Musae::ReconLGA
