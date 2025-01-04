#pragma once

#include "Musae/VisLGA/Type.h++"

#include "TCanvas.h"

#include "muc/ptrvec"

#include <memory>

namespace Musae::VisLGA {

auto EventPlot(const muc::shared_ptrvec<LGADigi>& coincidentDigi,
               const muc::shared_ptrvec<LGAHit>& eventHit,
               const CRMuEvent* cRMuEvent) -> std::unique_ptr<TCanvas>;

} // namespace Musae::VisLGA
