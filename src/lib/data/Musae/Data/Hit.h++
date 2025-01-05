#pragma once

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

namespace Musae::Data {

using LGAHit = Mustard::Data::TupleModel<
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<int, "HitID", "Hit ID">,
    Mustard::Data::Value<short, "ModID", "Hit module ID">,
    Mustard::Data::Value<float, "Edep", "Energy deposition">,
    Mustard::Data::Value<double, "t", "Hit time">,
    Mustard::Data::Value<double, "sigmaT", "Standard deviation of hit time">,
    Mustard::Data::Value<muc::array2f, "x", "Hit position">,
    Mustard::Data::Value<muc::array3f, "covX", "Covariance of x, i.e. [varX, varY, covXY]">,
    Mustard::Data::Value<muc::array2i16, "nLuminous", "Number of luminous WLS fibers per direction">>;

} // namespace Musae::Data
