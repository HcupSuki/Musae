#pragma once

#include "Musae/Data/Event.h++"

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

namespace Musae::Data {

using CRMuSimEvent = Mustard::Data::TupleModel<
    CRMuEvent,
    Mustard::Data::Value<int, "TrkID", "MC Track ID">,
    Mustard::Data::Value<int, "PDGID", "Particle PDG ID (MC truth)">,
    Mustard::Data::Value<float, "Ek0", "Vertex kinetic energy (MC truth)">,
    Mustard::Data::Value<muc::array3f, "p0", "Vertex momentum (MC truth)">,
    Mustard::Data::Value<float, "w", "Event weight">,
    Mustard::Data::Value<double, "Range", "Event range">>;

} // namespace Musae::Data
