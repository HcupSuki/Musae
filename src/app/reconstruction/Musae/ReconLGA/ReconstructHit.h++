#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/hash_map"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

auto ReconstructHit(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                    int hitID, std::string_view method) -> std::unique_ptr<LGAHit>;

} // namespace Musae::ReconLGA
