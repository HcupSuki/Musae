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

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include <vector>

class G4Event;

namespace Musae::GenCRMu {

using CRMuEvent = Mustard::Data::TupleModel<
    Mustard::Data::Value<double, "t", "Vertex time">,
    Mustard::Data::Value<float, "x", "Vertex x coordinate">,
    Mustard::Data::Value<float, "y", "Vertex y coordinate">,
    Mustard::Data::Value<float, "z", "Vertex z coordinate">,
    Mustard::Data::Value<std::vector<int>, "pdgID", "Primary particle">,
    Mustard::Data::Value<std::vector<float>, "px", "Primary momentum, x component">,
    Mustard::Data::Value<std::vector<float>, "py", "Primary momentum, y component">,
    Mustard::Data::Value<std::vector<float>, "pz", "Primary momentum, z component">,
    Mustard::Data::Value<float, "w", "Event weight">>;

auto BackProjection(const G4Event& g4Event, double elevation) -> Mustard::Data::Tuple<CRMuEvent>;

} // namespace Musae::GenCRMu
