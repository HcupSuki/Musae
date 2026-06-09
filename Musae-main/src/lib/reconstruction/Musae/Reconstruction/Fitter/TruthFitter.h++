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

#include "Musae/Data/SimEvent.h++"
#include "Musae/Data/SimHit.h++"
#include "Musae/Reconstruction/Fitter/FitterBase.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include <cmath>
#include <concepts>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace Musae::inline Reconstruction::inline Fitter {

template<Mustard::Data::SuperTupleModel<Data::LGASimHit> AHit = Data::LGASimHit,
         Mustard::Data::SuperTupleModel<Data::CRMuSimEvent> AEvent = Data::CRMuSimEvent>
class TruthFitter : public FitterBase<AHit, AEvent> {
public:
    using Hit = AHit;
    using Event = AEvent;

public:
    virtual ~TruthFitter() = default;

    auto CheckHitDataConsistency() const -> auto { return fCheckHitDataConsistency; }
    auto CheckHitDataConsistency(bool val) -> void { fCheckHitDataConsistency = val; }

    template<std::indirectly_readable AHitPointer>
        requires Mustard::Data::SuperTupleModel<typename std::iter_value_t<AHitPointer>::Model, AHit>
    auto operator()(const std::vector<AHitPointer>& hitData) -> std::shared_ptr<Mustard::Data::Tuple<AEvent>>;

private:
    bool fCheckHitDataConsistency{true};
};

} // namespace Musae::inline Reconstruction::inline Fitter

#include "Musae/Reconstruction/Fitter/TruthFitter.inl"
