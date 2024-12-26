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
    Mustard::Data::Value<float, "normalizedEnergy">>;

} // namespace Musae::Data
