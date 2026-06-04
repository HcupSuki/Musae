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
