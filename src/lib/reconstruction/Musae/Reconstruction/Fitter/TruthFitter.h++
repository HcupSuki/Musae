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
