#pragma once

#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include "RtypesCore.h"

namespace Musae::Data {

using LGARawDigi = Mustard::Data::TupleModel<
    Mustard::Data::Value<Long64_t, "time", "Trigger time (in digitizer time unit)">,
    Mustard::Data::Value<unsigned, "channelID", "Channel ID">,
    Mustard::Data::Value<float, "energy", "Digitized energy (in digitizer energy unit)">>;

using LGADigi = Mustard::Data::TupleModel<
    LGARawDigi,
    Mustard::Data::Value<int, "EvtID", "Event ID">,
    Mustard::Data::Value<Long64_t, "t0", "Event begin time (in digitizer time unit)">,
    Mustard::Data::Value<float, "t", "Trigger time relative to t0">,
    Mustard::Data::Value<bool, "Good", "Trigger selection flag">,
    Mustard::Data::Value<short, "ModID", "Module ID">,
    Mustard::Data::Value<char, "Edge", "Edge of this digi">,
    Mustard::Data::Value<short, "FibLocID", "Fiber local ID">,
    Mustard::Data::Value<float, "NormalizedEnergy", "Normalized energy">>;

} // namespace Musae::Data
