#pragma once

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "RtypesCore.h"

namespace Musae::Data {

using LGADigi = Mustard::Data::TupleModel<
    Mustard::Data::Value<Long64_t, "time">,
    Mustard::Data::Value<unsigned, "channelID">,
    Mustard::Data::Value<float, "energy">>;

} // namespace Musae::Data
