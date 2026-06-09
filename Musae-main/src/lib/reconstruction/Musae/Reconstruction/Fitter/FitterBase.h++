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
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Data/TupleModel.h++"

#include <memory>
#include <vector>

namespace Musae::inline Reconstruction::inline Fitter {

template<Mustard::Data::SuperTupleModel<Data::LGAHit> AHit,
         Mustard::Data::SuperTupleModel<Data::CRMuEvent> AEvent>
class FitterBase {
public:
    using Hit = AHit;
    using Event = AEvent;

public:
    FitterBase();
    virtual ~FitterBase() = default;

    auto MinNHit() const -> auto { return fMinNHit; }
    auto MinNHit(int n) -> void { fMinNHit = std::max(1, n); }

private:
    int fMinNHit;
};

} // namespace Musae::inline Reconstruction::inline Fitter

#include "Musae/Reconstruction/Fitter/FitterBase.inl"
