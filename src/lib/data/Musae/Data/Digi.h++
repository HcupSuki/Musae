#pragma once

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "RtypesCore.h"

namespace Musae::Data {

using LGARawDigi = Mustard::Data::TupleModel<
    Mustard::Data::Value<double, "time">,
    Mustard::Data::Value<unsigned, "channelID">,
    Mustard::Data::Value<float, "energy">>;

using LGADigi = Mustard::Data::TupleModel<
    LGARawDigi,
    Mustard::Data::Value<int, "EvtID">,
    Mustard::Data::Value<short, "ModID">,
    Mustard::Data::Value<char, "Edge">,
    Mustard::Data::Value<short, "FibLocID">,
    Mustard::Data::Value<float, "NormalizedEnergy">>;

} // namespace Musae::Data
