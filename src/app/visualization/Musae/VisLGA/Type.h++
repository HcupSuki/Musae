#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"

#include "Mustard/Data/Tuple.h++"

#include "muc/hash_map"

#include <memory>
#include <vector>

namespace Musae::VisLGA {

using LGADigi = Mustard::Data::Tuple<Musae::Data::LGADigi>;
using LGAHit = Mustard::Data::Tuple<Musae::Data::LGAHit>;
using CRMuEvent = Mustard::Data::Tuple<Musae::Data::CRMuEvent>;

template<typename AData>
using DataVector = std::vector<std::shared_ptr<AData>>;
/// @brief A data structure like {module ID, edge} -> digi.
/// Use it like data.at(moduleID).at(edge), e.g. data.at(1).at('x')
/// or data[moduleID][edge], e.g. data[1][x], for insertion.
/// @tparam ADigi An LGA digi object or pointer.
template<typename ADigi>
using LGADigiMap = muc::flat_hash_map<int, muc::flat_hash_map<char, std::vector<ADigi>>>;

} // namespace Musae::VisLGA
