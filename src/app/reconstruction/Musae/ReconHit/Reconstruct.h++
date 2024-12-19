#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Hit.h++"

#include "Mustard/Data/Tuple.h++"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconHit {

auto Reconstruct(const std::unordered_map<char, std::vector<const Mustard::Data::Tuple<Data::LGADigi>*>>& digiData,
                 int eventID, int hitID, std::string_view method) -> std::unique_ptr<Mustard::Data::Tuple<Data::LGAHit>>;

} // namespace Musae::ReconHit
