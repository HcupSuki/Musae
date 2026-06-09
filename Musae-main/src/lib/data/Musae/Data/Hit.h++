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

namespace Musae::Data {

using LGAHit = Mustard::Data::TupleModel<
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<int, "DetID", "Detector ID">,
    Mustard::Data::Value<int, "HitID", "Hit ID">,
    Mustard::Data::Value<short, "ModID", "Hit module ID">,
    Mustard::Data::Value<float, "Edep", "Energy deposition">,
    Mustard::Data::Value<double, "t", "Hit time">,
    Mustard::Data::Value<double, "sigmaT", "Standard deviation of hit time">,
    Mustard::Data::Value<muc::array2f, "x", "Hit position">,
    Mustard::Data::Value<muc::array3f, "covX", "Covariance of x, i.e. [varX, varY, covXY]">,
    Mustard::Data::Value<muc::array2i16, "nLuminous", "Number of luminous WLS fibers per direction">>;

} // namespace Musae::Data
