#pragma once

#include "Musae/Data/Event.h++"
#include "Musae/ReconLGA/Type.h++"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

auto ReconstructCRMu(const std::vector<std::unique_ptr<LGAHit>>& eventHit, std::string_view method) -> std::unique_ptr<CRMuEvent>;

} // namespace Musae::ReconLGA
