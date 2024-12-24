#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Hit.h++"

#include "Mustard/Data/Tuple.h++"

#include <unordered_map>

namespace Musae::ReconHit {

using LGADigi = Mustard::Data::Tuple<Musae::Data::LGADigi>;
using LGAHit = Mustard::Data::Tuple<Musae::Data::LGAHit>;

/// @brief A data structure like {module ID, edge} -> digi.
/// Use it like data.at(moduleID).at(edge), e.g. data.at(1).at('x')
/// or data[moduleID][edge], e.g. data[1][x], for insertion.
/// @tparam ADigi An LGA digi object or pointer.
template<typename ADigi>
using LGADigiMap = std::unordered_map<int, std::unordered_map<char, std::vector<ADigi>>>;

} // namespace Musae::ReconHit
