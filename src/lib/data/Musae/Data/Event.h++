#pragma once

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

#include <vector>

namespace Musae::Data {

using CRMuEvent = Mustard::Data::TupleModel<
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<std::vector<int>, "HitID", "Hit(ID)s in this track">,
    Mustard::Data::Value<float, "chi2", "Goodness of fit (chi^{2})">,
    Mustard::Data::Value<float, "MAE", "Mean absolute error between hit to track">,
    Mustard::Data::Value<double, "t0", "Event time">,
    Mustard::Data::Value<muc::array3f, "x0", "A position on track">,
    Mustard::Data::Value<float, "theta", "Event zenith angle">,
    Mustard::Data::Value<float, "phi", "Event azimuth angle">>;

} // namespace Musae::Data
