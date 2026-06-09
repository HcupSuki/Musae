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

#include "RtypesCore.h"

namespace Musae::Data {

using LGARawDigi = Mustard::Data::TupleModel<
    Mustard::Data::Value<Long64_t, "time", "Trigger time (in digitizer time unit)">,
    Mustard::Data::Value<unsigned, "channelID", "Channel ID">,
    Mustard::Data::Value<float, "energy", "Digitized energy (in digitizer energy unit)">>;

using LGADigi = Mustard::Data::TupleModel<
    LGARawDigi,
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<Long64_t, "t0", "Event begin time (in digitizer time unit)">,
    Mustard::Data::Value<float, "t", "Trigger time relative to t0">,
    Mustard::Data::Value<bool, "Good", "Trigger selection flag">,
    Mustard::Data::Value<short, "ModID", "Module ID">,
    Mustard::Data::Value<char, "Edge", "Edge of this digi">,
    Mustard::Data::Value<short, "FibLocID", "Fiber local ID">,
    Mustard::Data::Value<float, "NormalizedEnergy", "Normalized energy">>;

} // namespace Musae::Data
