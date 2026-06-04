#pragma once

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Data/TupleModel.h++"
#include "Mustard/Data/Value.h++"

#include <vector>

class G4Event;

namespace Musae::GenCRMu {

using CRMuEvent = Mustard::Data::TupleModel<
    Mustard::Data::Value<double, "t", "Vertex time">,
    Mustard::Data::Value<float, "x", "Vertex x coordinate">,
    Mustard::Data::Value<float, "y", "Vertex y coordinate">,
    Mustard::Data::Value<float, "z", "Vertex z coordinate">,
    Mustard::Data::Value<std::vector<int>, "pdgID", "Primary particle">,
    Mustard::Data::Value<std::vector<float>, "px", "Primary momentum, x component">,
    Mustard::Data::Value<std::vector<float>, "py", "Primary momentum, y component">,
    Mustard::Data::Value<std::vector<float>, "pz", "Primary momentum, z component">,
    Mustard::Data::Value<float, "w", "Event weight">>;

auto BackProjection(const G4Event& g4Event, double elevation) -> Mustard::Data::Tuple<CRMuEvent>;

} // namespace Musae::GenCRMu
