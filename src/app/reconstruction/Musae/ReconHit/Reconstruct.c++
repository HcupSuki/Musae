#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconHit/Reconstruct.h++"

#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/VectorArithmeticOperator.h++"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

#include "fmt/format.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Musae::ReconHit {

namespace internal {
namespace {

using namespace Mustard::VectorArithmeticOperator;

auto Weighted2D(const std::unordered_map<char, std::vector<const Mustard::Data::Tuple<Data::LGADigi>*>>& digiData)
    -> std::unique_ptr<Mustard::Data::Tuple<Data::LGAHit>> {
    const auto& lga{Detector::Description::LGA::Instance()};

    double time{};
    double totalEnergy{};
    for (auto&& [edge, digiPerEdge] : digiData) {
        for (auto&& digi : std::as_const(digiPerEdge)) {
            const auto energy{Get<"energy">(*digi)};
            time += energy * (Get<"time">(*digi) * CLHEP::ps);
            totalEnergy += energy;
        }
    }
    time /= totalEnergy;

    CLHEP::Hep2Vector position{};
    double positionWeight{};
    for (auto&& xDigi : digiData.at('x')) {
        for (auto&& yDigi : digiData.at('y')) {
            const auto intersection{lga.Intersection(Get<"channelID">(*xDigi), Get<"channelID">(*yDigi))};
            const auto weight{Get<"energy">(*xDigi) + Get<"energy">(*yDigi)};
            position += weight * intersection;
            positionWeight += weight;
        }
    }
    position /= positionWeight;

    auto hit{std::make_unique_for_overwrite<Mustard::Data::Tuple<Data::LGAHit>>()};
    Get<"t">(*hit) = time;
    Get<"x">(*hit) = position;
    Get<"Edep">(*hit) = totalEnergy;
    return hit;
}

} // namespace
} // namespace internal

auto Reconstruct(const std::unordered_map<char, std::vector<const Mustard::Data::Tuple<Data::LGADigi>*>>& digiData,
                 int eventID, int hitID, std::string_view method) -> std::unique_ptr<Mustard::Data::Tuple<Data::LGAHit>> {
    const auto& lga{Detector::Description::LGA::Instance()};
    const auto moduleID{lga.ChannelInfo(Get<"channelID">(*digiData.at('x').front())).moduleID};
    for (auto&& [_, digi] : digiData) {
        if (std::ranges::any_of(digi, [&](auto&& d) { return lga.ChannelInfo(Get<"channelID">(*d)).moduleID != moduleID; })) {
            Mustard::Throw<std::runtime_error>("Module IDs in digi data are not all the same");
        }
    }

    muc::array2i16 nLuminous{static_cast<std::int16_t>(digiData.at('x').size()),
                             static_cast<std::int16_t>(digiData.at('y').size())};
    if (nLuminous[0] < lga.NLuminousDigiThresholdPerDirection() or
        nLuminous[1] < lga.NLuminousDigiThresholdPerDirection()) {
        return nullptr;
    }

    std::unique_ptr<Mustard::Data::Tuple<Data::LGAHit>> hit;
    if (method == "Weighted2D") {
        hit = internal::Weighted2D(digiData);
    } else {
        Mustard::Throw<std::runtime_error>(fmt::format("No method named '{}'", method));
    }
    if (hit == nullptr) {
        return hit;
    }

    Get<"EvtID">(*hit) = eventID;
    Get<"HitID">(*hit) = hitID;
    Get<"ModID">(*hit) = moduleID;
    Get<"nLuminous">(*hit) = nLuminous;
    return hit;
}

} // namespace Musae::ReconHit
