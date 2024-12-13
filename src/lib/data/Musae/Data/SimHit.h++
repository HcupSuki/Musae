#pragma once

#include "Musae/Data/Hit.h++"

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "muc/array"

namespace Musae::Data {

using LGASimHit = Mustard::Data::TupleModel<
    LGAHit,
    Mustard::Data::Value<int, "TrkID", "MC Track ID">,
    Mustard::Data::Value<int, "PDGID", "Particle PDG ID (MC truth)">,
    Mustard::Data::Value<float, "Ek", "Kinetic energy (MC truth)">,
    Mustard::Data::Value<muc::array3f, "p", "Momentum (MC truth)">,
    Mustard::Data::Value<float, "w", "Event weight">>;

} // namespace Musae::Data
