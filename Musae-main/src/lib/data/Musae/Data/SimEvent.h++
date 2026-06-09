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

#include "Musae/Data/Event.h++"

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

namespace Musae::Data {

using CRMuSimEvent = Mustard::Data::TupleModel<
    CRMuEvent,
    Mustard::Data::Value<int, "TrkID", "MC Track ID">,
    Mustard::Data::Value<int, "PDGID", "Particle PDG ID (MC truth)">,
    Mustard::Data::Value<float, "Ek0", "Vertex kinetic energy (MC truth)">,
    Mustard::Data::Value<muc::array3f, "p0", "Vertex momentum (MC truth)">,
    Mustard::Data::Value<float, "w", "Event weight">,
    Mustard::Data::Value<double, "Range", "Event range">>;

} // namespace Musae::Data
