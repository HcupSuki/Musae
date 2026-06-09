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

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Data/TupleModel.h++"

#include <concepts>
#include <iterator>
#include <memory>
#include <utility>
#include <vector>

namespace Musae::inline Reconstruction::inline Fitter {

template<typename T>
concept Fitter =
    requires {
        typename T::Hit;
        typename T::Event;
        requires Mustard::Data::SuperTupleModel<typename T::Hit, Data::LGAHit>;
        requires Mustard::Data::SuperTupleModel<typename T::Event, Data::CRMuEvent>;
    } and
    requires(T fitter, const std::vector<Mustard::Data::Tuple<typename T::Hit>*> hitData) {
        { fitter(hitData) };
        { fitter(hitData).track } -> std::same_as<std::shared_ptr<Mustard::Data::Tuple<typename T::Event>>>;
        { fitter(hitData).fitted } -> std::same_as<std::vector<Mustard::Data::Tuple<typename T::Hit>*>>;
        { fitter(hitData).failed } -> std::same_as<std::vector<Mustard::Data::Tuple<typename T::Hit>*>>;
    } and
    requires(T fitter, const std::vector<std::shared_ptr<Mustard::Data::Tuple<typename T::Hit>>> hitData) {
        { fitter(hitData) };
        { fitter(hitData).track } -> std::same_as<std::shared_ptr<Mustard::Data::Tuple<typename T::Event>>>;
        { fitter(hitData).fitted } -> std::same_as<std::vector<std::shared_ptr<Mustard::Data::Tuple<typename T::Hit>>>>;
        { fitter(hitData).failed } -> std::same_as<std::vector<std::shared_ptr<Mustard::Data::Tuple<typename T::Hit>>>>;
    };

template<typename T>
concept SimFitter =
    requires {
        requires Fitter<T>;
        requires Mustard::Data::SuperTupleModel<typename T::Hit, Data::LGASimHit>;
        requires Mustard::Data::SuperTupleModel<typename T::Event, Data::CRMuSimEvent>;
    };

} // namespace Musae::inline Reconstruction::inline Fitter
