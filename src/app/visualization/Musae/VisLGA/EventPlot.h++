#pragma once

#include "Musae/VisLGA/Type.h++"

#include "TCanvas.h"

#include <memory>

namespace Musae::VisLGA {

auto EventPlot(const DataVector<LGADigi>& coincidentDigi,
               const DataVector<LGAHit>& eventHit,
               const CRMuEvent* cRMuEvent) -> std::unique_ptr<TCanvas>;

} // namespace Musae::VisLGA
