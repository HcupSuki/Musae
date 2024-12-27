#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"

#include "Mustard/Math/Statistic.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

#include "muc/ceta_string"

#include "fmt/format.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace Musae::ReconLGA {

namespace {
namespace HitReconstruction {

namespace internal {

template<muc::ceta_string AWeight>
auto Weighted2D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    const auto& lga{Detector::Description::LGA::Instance()};

    Mustard::Math::Statistic<1> time;
    for (auto&& [edge, digiPerEdge] : digiData) {
        for (auto&& digi : std::as_const(digiPerEdge)) {
            time.Fill(Get<"time">(*digi), Get<AWeight>(*digi));
        }
    }

    Mustard::Math::Statistic<2> position;
    for (auto&& xDigi : digiData.at('x')) {
        for (auto&& yDigi : digiData.at('y')) {
            position.Fill(lga.Intersection(Get<"channelID">(*xDigi), Get<"channelID">(*yDigi)),
                          Get<AWeight>(*xDigi) + Get<AWeight>(*yDigi));
        }
    }

    auto hit{std::make_unique_for_overwrite<LGAHit>()};
    Get<"t">(*hit) = time.Mean();
    Get<"x">(*hit) = position.Mean();
    Get<"covX">(*hit) = {static_cast<float>(position.Variance(0)),
                         static_cast<float>(position.Variance(1)),
                         static_cast<float>(position.Covariance(0, 1))};
    return hit;
}

} // namespace internal

auto EnergyWeighted2D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted2D<"energy">(digiData);
}

auto NormalizedEnergyWeighted2D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted2D<"NormalizedEnergy">(digiData);
}

} // namespace HitReconstruction
} // namespace

auto ReconstructHit(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                    int hitID, std::string_view method) -> std::unique_ptr<LGAHit> {
    const auto& lga{Detector::Description::LGA::Instance()};
    const auto moduleID{*Get<"ModID">(*digiData.at('x').front())};
    for (auto&& [_, digi] : digiData) {
        if (std::ranges::any_of(digi, [&](auto&& d) { return Get<"ModID">(*d) != moduleID; })) {
            Mustard::Throw<std::runtime_error>("Module IDs in digi data are not all the same");
        }
    }
    const auto eventID{*Get<"EvtID">(*digiData.at('x').front())};
    for (auto&& [_, digi] : digiData) {
        if (std::ranges::any_of(digi, [&](auto&& d) { return Get<"EvtID">(*d) != eventID; })) {
            Mustard::Throw<std::runtime_error>("Event IDs in digi data are not all the same");
        }
    }

    muc::array2i16 nLuminous{static_cast<std::int16_t>(digiData.at('x').size()),
                             static_cast<std::int16_t>(digiData.at('y').size())};
    if (nLuminous[0] < lga.NLuminousDigiThresholdPerDirection() or
        nLuminous[1] < lga.NLuminousDigiThresholdPerDirection()) {
        return nullptr;
    }

    std::unique_ptr<LGAHit> hit;
    if (method == "EnergyWeighted2D") {
        hit = HitReconstruction::EnergyWeighted2D(digiData);
    } else if (method == "NormalizedEnergyWeighted2D") {
        hit = HitReconstruction::NormalizedEnergyWeighted2D(digiData);
    } else {
        Mustard::Throw<std::runtime_error>(fmt::format("No method named '{}'", method));
    }
    if (hit == nullptr) {
        return hit;
    }

    double eDep{};
    for (auto&& [_, digiPerEdge] : digiData) {
        for (auto&& digi : std::as_const(digiPerEdge)) {
            eDep += Get<"energy">(*digi);
        }
    }

    Get<"EvtID">(*hit) = eventID;
    Get<"HitID">(*hit) = hitID;
    Get<"ModID">(*hit) = moduleID;
    Get<"Edep">(*hit) = eDep;
    Get<"nLuminous">(*hit) = nLuminous;
    return hit;
}

} // namespace Musae::ReconLGA
