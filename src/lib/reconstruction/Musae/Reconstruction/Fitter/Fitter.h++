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
