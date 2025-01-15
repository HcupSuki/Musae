#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"

#include "Mustard/Math/Statistic.h++"
#include "Mustard/Utility/PhysicalConstant.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

#include "Math/WrappedFunction.h"
#include "Minuit2/Minuit2Minimizer.h"

#include "Eigen/Dense"

#include "muc/ceta_string"
#include "muc/math"

#include "gsl/gsl"

#include "fmt/format.h"

#include <algorithm>
#include <cmath>
#include <concepts>
#include <limits>
#include <queue>
#include <stdexcept>
#include <tuple>
#include <unordered_map>
#include <utility>

namespace Musae::ReconLGA {

auto ParseReconstructHitMethod(std::string_view method) -> ReconstructHitMethod {
    static const std::unordered_map<std::string_view, ReconstructHitMethod> methodMap{
        {"SLinTW1D",          ReconstructHitMethod::SLinTW1D         },
        {"SLinTW2D",          ReconstructHitMethod::SLinTW2D         },
        {"EW1D",              ReconstructHitMethod::EW1D             },
        {"NEW1D",             ReconstructHitMethod::NEW1D            },
        {"SLinTEW1D",         ReconstructHitMethod::SLinTEW1D        },
        {"SLinTNEW1D",        ReconstructHitMethod::SLinTNEW1D       },
        {"SLinTTopW1D",       ReconstructHitMethod::SLinTTopW1D      },
        {"ETopW1D",           ReconstructHitMethod::ETopW1D          },
        {"NETopW1D",          ReconstructHitMethod::NETopW1D         },
        {"SLinTETopW1D",      ReconstructHitMethod::SLinTETopW1D     },
        {"SLinTNETopW1D",     ReconstructHitMethod::SLinTNETopW1D    },
        {"SLinTW1DCombEW1D",  ReconstructHitMethod::SLinTW1DCombEW1D },
        {"SLinTW1DCombNEW1D", ReconstructHitMethod::SLinTW1DCombNEW1D}
    };
    try {
        return methodMap.at(method);
    } catch (const std::out_of_range&) {
        Mustard::Throw<std::runtime_error>(fmt::format("No method named '{}'", method));
    }
}

namespace {
namespace HitReconstruction {

using namespace Mustard::PhysicalConstant;

namespace internal {

auto WeightedTime(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                  std::invocable<const LGADigi&> auto&& Weight) -> auto {
    Mustard::Math::Statistic<1> time;
    for (auto&& [_, digiPerEdge] : digiData) {
        for (auto&& digi : std::as_const(digiPerEdge)) {
            time.Fill(Get<"t">(*digi), Weight(*digi));
        }
    }
    return time;
}

template<muc::ceta_string ACovMode>
auto Weighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                std::invocable<const LGADigi&> auto&& Weight) -> std::unique_ptr<LGAHit> {
    const auto& lga{Detector::Description::LGA::Instance()};

    const auto time{WeightedTime(digiData, Weight)};
    Mustard::Math::Statistic<1> x;
    for (auto&& digi : digiData.at('x')) {
        x.Fill(lga.ChannelInfo(Get<"channelID">(*digi)).edgePosition, Weight(*digi));
    }
    Mustard::Math::Statistic<1> y;
    for (auto&& digi : digiData.at('y')) {
        y.Fill(lga.ChannelInfo(Get<"channelID">(*digi)).edgePosition, Weight(*digi));
    }

    auto hit{std::make_unique_for_overwrite<LGAHit>()};
    Get<"t">(*hit) = time.Mean() + Get<"t0">(*digiData.at('x').front()) * CLHEP::ps;
    Get<"sigmaT">(*hit) = time.StdDev();
    Get<"x">(*hit) = {static_cast<float>(x.Mean()), static_cast<float>(y.Mean())};
    if constexpr (ACovMode == "Variance") {
        Get<"covX">(*hit) = {static_cast<float>(x.Variance()), static_cast<float>(y.Variance()), 0.f};
    } else if constexpr (ACovMode == "VarianceOfMean") {
        Get<"covX">(*hit) = {static_cast<float>(x.VarianceOfMean()), static_cast<float>(y.VarianceOfMean()), 0.f};
    } else {
        static_assert(false, "No such covariance mode");
    }
    return hit;
}

template<muc::ceta_string ACovMode>
auto TopWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                   std::invocable<const LGADigi&> auto&& Weight) -> std::unique_ptr<LGAHit> {
    const auto TopEdgeDigiData{[&](const std::vector<LGADigi*>& edgeDigi) {
        if (edgeDigi.size() < 3) {
            Mustard::Throw<std::invalid_argument>("There should be at least 3 digi-s per edge");
        }
        std::vector<LGADigi*> topDigi(3);
        std::ranges::partial_sort_copy(
            edgeDigi, topDigi,
            [&](auto&& digi1, auto&& digi2) {
                return Weight(*digi1) > Weight(*digi2);
            });
        return topDigi;
    }};
    muc::flat_hash_map<char, std::vector<LGADigi*>> topDigiData;
    topDigiData.reserve(2);
    topDigiData.emplace('x', TopEdgeDigiData(digiData.at('x')));
    topDigiData.emplace('y', TopEdgeDigiData(digiData.at('y')));
    return Weighted1D<ACovMode>(topDigiData, std::forward<decltype(Weight)>(Weight));
}

template<muc::ceta_string ACovMode>
auto Weighted2D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                std::invocable<const LGADigi&> auto&& Weight) -> std::unique_ptr<LGAHit> {
    const auto& lga{Detector::Description::LGA::Instance()};

    const auto time{WeightedTime(digiData, Weight)};
    Mustard::Math::Statistic<2> position;
    for (auto&& xDigi : digiData.at('x')) {
        for (auto&& yDigi : digiData.at('y')) {
            position.Fill(lga.Intersection(Get<"channelID">(*xDigi), Get<"channelID">(*yDigi)),
                          std::sqrt(muc::pow<2>(Weight(*xDigi)) + muc::pow<2>(Weight(*yDigi))));
        }
    }

    auto hit{std::make_unique_for_overwrite<LGAHit>()};
    Get<"t">(*hit) = time.Mean() + Get<"t0">(*digiData.at('x').front()) * CLHEP::ps;
    Get<"sigmaT">(*hit) = time.StdDev();
    Get<"x">(*hit) = position.Mean();
    if constexpr (ACovMode == "Variance") {
        Get<"covX">(*hit) = {static_cast<float>(position.Variance(0)),
                             static_cast<float>(position.Variance(1)),
                             static_cast<float>(position.Covariance(0, 1))};
    } else if constexpr (ACovMode == "VarianceOfMean") {
        Get<"covX">(*hit) = {static_cast<float>(position.VarianceOfMean(0)),
                             static_cast<float>(position.VarianceOfMean(1)),
                             static_cast<float>(position.CovarianceOfMean(0, 1))};
    } else {
        static_assert(false, "No such covariance mode");
    }
    return hit;
}

auto SoftLinearTimeWeight(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> auto {
    auto t0{std::numeric_limits<double>::max()};
    for (auto&& [_, digiPerEdge] : digiData) {
        for (auto&& digi : std::as_const(digiPerEdge)) {
            if (Get<"t">(*digi) < t0) {
                t0 = Get<"t">(*digi);
            }
        }
    }
    return [&, aPrioriSigmaT = WeightedTime(digiData, [](auto&&) { return 1; })
                                   .StdDev()](auto&& digi) {
        return 1 / std::sqrt(1 + muc::pow<2>((Get<"t">(digi) - t0) / aPrioriSigmaT));
    };
}

} // namespace internal

auto SoftLinearTimeWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted1D<"VarianceOfMean">(digiData, internal::SoftLinearTimeWeight(digiData));
}

template<muc::ceta_string AWeight>
auto SoftLinearTimeWeighted2D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted2D<"VarianceOfMean">(digiData, internal::SoftLinearTimeWeight(digiData));
}

template<muc::ceta_string AWeight>
auto EnergyWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted1D<"Variance">(digiData, [](auto&& digi) { return Get<AWeight>(digi); });
}

template<muc::ceta_string AWeight>
auto SoftLinearTimeEnergyWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::Weighted1D<"Variance">(
        digiData, [TimeWeight = internal::SoftLinearTimeWeight(digiData)](auto&& digi) {
            return TimeWeight(digi) * Get<AWeight>(digi);
        });
}

auto SoftLinearTimeTopWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::TopWeighted1D<"VarianceOfMean">(digiData, internal::SoftLinearTimeWeight(digiData));
}

template<muc::ceta_string AWeight>
auto EnergyTopWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::TopWeighted1D<"Variance">(digiData, [](auto&& digi) { return Get<AWeight>(digi); });
}

template<muc::ceta_string AWeight>
auto SoftLinearTimeEnergyTopWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    return internal::TopWeighted1D<"Variance">(
        digiData, [TimeWeight = internal::SoftLinearTimeWeight(digiData)](auto&& digi) {
            return TimeWeight(digi) * Get<AWeight>(digi);
        });
}

template<muc::ceta_string AWeight>
auto SoftLinearTimeWeighted1DCombineEnergyWeighted1D(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData) -> std::unique_ptr<LGAHit> {
    const auto hit1{SoftLinearTimeWeighted1D(digiData)};
    const auto hit2{EnergyWeighted1D<AWeight>(digiData)};
    auto hit{std::make_unique_for_overwrite<LGAHit>()};

    // time is highly correlated; just use one of it
    Get<"t">(*hit) = Get<"t">(*hit1);
    Get<"sigmaT">(*hit) = Get<"sigmaT">(*hit1);

    // combine position
    const auto PositionCovariance{[](auto&& hit) {
        const auto [varX, varY, covXY]{*Get<"covX">(*hit)};
        return Eigen::Matrix2d{
            {varX,  covXY},
            {covXY, varY }
        };
    }};
    const auto covX1{PositionCovariance(hit1)};
    const auto covX2{PositionCovariance(hit2)};
    const auto wX1{covX1.inverse().eval()};
    const auto wX2{covX2.inverse().eval()};
    const auto inverseWX1PlusWX2{(wX1 + wX2).inverse().eval()};
    Get<"x">(*hit) = (inverseWX1PlusWX2 *
                      (wX1 * GetAs<"x", Eigen::Vector2d>(*hit1) +
                       wX2 * GetAs<"x", Eigen::Vector2d>(*hit2)))
                         .eval();
    const auto jacX1{(inverseWX1PlusWX2 * wX1).eval()};
    const auto jacX2{(inverseWX1PlusWX2 * wX2).eval()};
    const auto covX{(jacX1.transpose() * covX1 * jacX1 + jacX2.transpose() * covX2 * jacX2).eval()};
    Get<"covX">(*hit) = {static_cast<float>(covX(0, 0)),
                         static_cast<float>(covX(1, 1)),
                         static_cast<float>(covX(0, 1))};

    return hit;
}

} // namespace HitReconstruction
} // namespace

auto ReconstructHit(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                    int hitID, ReconstructHitMethod method) -> std::unique_ptr<LGAHit> {
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

    auto hit{[&]() -> std::unique_ptr<LGAHit> {
        switch (method) {
        case ReconstructHitMethod::SLinTW1D:
            return HitReconstruction::SoftLinearTimeWeighted1D(digiData);
        case ReconstructHitMethod::EW1D:
            return HitReconstruction::EnergyWeighted1D<"energy">(digiData);
        case ReconstructHitMethod::NEW1D:
            return HitReconstruction::EnergyWeighted1D<"NormalizedEnergy">(digiData);
        case ReconstructHitMethod::SLinTEW1D:
            return HitReconstruction::SoftLinearTimeEnergyWeighted1D<"energy">(digiData);
        case ReconstructHitMethod::SLinTNEW1D:
            return HitReconstruction::SoftLinearTimeEnergyWeighted1D<"NormalizedEnergy">(digiData);
        case ReconstructHitMethod::SLinTTopW1D:
            return HitReconstruction::SoftLinearTimeTopWeighted1D(digiData);
        case ReconstructHitMethod::ETopW1D:
            return HitReconstruction::EnergyTopWeighted1D<"energy">(digiData);
        case ReconstructHitMethod::NETopW1D:
            return HitReconstruction::EnergyTopWeighted1D<"NormalizedEnergy">(digiData);
        case ReconstructHitMethod::SLinTETopW1D:
            return HitReconstruction::SoftLinearTimeEnergyTopWeighted1D<"energy">(digiData);
        case ReconstructHitMethod::SLinTNETopW1D:
            return HitReconstruction::SoftLinearTimeEnergyTopWeighted1D<"NormalizedEnergy">(digiData);
        case ReconstructHitMethod::SLinTW1DCombEW1D:
            return HitReconstruction::SoftLinearTimeWeighted1DCombineEnergyWeighted1D<"energy">(digiData);
        case ReconstructHitMethod::SLinTW1DCombNEW1D:
            return HitReconstruction::SoftLinearTimeWeighted1DCombineEnergyWeighted1D<"NormalizedEnergy">(digiData);
        default:
            return nullptr;
        }
    }()};
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
