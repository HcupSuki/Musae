// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"

#include "Mustard/Data/Tuple.h++"

#include "muc/hash_map"

namespace Musae::ReconLGA {

using LGARawDigi = Mustard::Data::Tuple<Musae::Data::LGARawDigi>;
using LGADigi = Mustard::Data::Tuple<Musae::Data::LGADigi>;
using LGAHit = Mustard::Data::Tuple<Musae::Data::LGAHit>;
using CRMuEvent = Mustard::Data::Tuple<Musae::Data::CRMuEvent>;

/// @brief A data structure like {module ID, edge} -> digi.
/// Use it like data.at(moduleID).at(edge), e.g. data.at(1).at('x')
/// or data[moduleID][edge], e.g. data[1][x], for insertion.
/// @tparam ADigi An LGA digi object or pointer.
template<typename ADigi>
using LGADigiMap = muc::flat_hash_map<int, muc::flat_hash_map<char, std::vector<ADigi>>>;

} // namespace Musae::ReconLGA
