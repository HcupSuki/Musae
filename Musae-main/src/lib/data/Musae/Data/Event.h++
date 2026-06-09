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

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

#include <vector>

namespace Musae::Data {

using CRMuEvent = Mustard::Data::TupleModel<
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<int, "DetID", "Detector ID">,
    Mustard::Data::Value<std::vector<int>, "HitID", "Hit(ID)s in this track">,
    Mustard::Data::Value<float, "chi2", "Goodness of fit (chi^{2})">,
    Mustard::Data::Value<float, "MAE", "Mean absolute error between hit to track">,
    Mustard::Data::Value<double, "t0", "Event time">,
    Mustard::Data::Value<muc::array3f, "x0", "A position on track">,
    Mustard::Data::Value<float, "theta", "Event zenith angle">,
    Mustard::Data::Value<float, "phi", "Event azimuth angle">,
    Mustard::Data::Value<float, "Edep", "Energy deposition">>;

} // namespace Musae::Data
