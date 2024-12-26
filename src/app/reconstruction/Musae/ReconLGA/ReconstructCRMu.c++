#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructCRMu.h++"

#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Vector/ThreeVector.h"

#include "Math/WrappedFunction.h"
#include "Minuit2/Minuit2Minimizer.h"

#include "Eigen/Dense"

#include "muc/math"
#include "muc/numeric"

#include <algorithm>
#include <functional>
#include <tuple>

namespace Musae::ReconLGA {

namespace {
namespace CRMuReconstruction {

namespace internal {

auto LeastSquare(const std::vector<std::unique_ptr<LGAHit>>& eventHit,
                 const ROOT::Math::IMultiGenFunction& Square) -> std::unique_ptr<CRMuEvent> {
    const auto& lga{Detector::Description::LGA::Instance()};
    const auto xScale{muc::midpoint(lga.ScintillatorWidthX(), lga.ScintillatorWidthY())};

    ROOT::Minuit2::Minuit2Minimizer minimizer;
    minimizer.SetVariable(0, "x0", 0, 0.001 * xScale);
    minimizer.SetVariable(1, "y0", 0, 0.001 * xScale);
    minimizer.SetVariable(2, "dX", 0, 0.001);
    minimizer.SetVariable(3, "dY", 0, 0.001);
    minimizer.SetFunction(Square);
    minimizer.Minimize();
    const auto minimum{minimizer.State()};
    if (not minimum.IsValid()) {
        return nullptr;
    }

    const CLHEP::Hep3Vector x0{minimum.Value(0), minimum.Value(1), 0};
    const CLHEP::Hep3Vector d{minimum.Value(2), minimum.Value(3), 1};
    const auto t0{muc::ranges::transform_reduce(
                      eventHit, 0., std::plus{},
                      [](auto&& h) { return Get<"t">(*h); }) /
                  eventHit.size()};

    auto event{std::make_unique_for_overwrite<CRMuEvent>()};
    for (auto&& hit : eventHit) { Get<"HitID">(*event)->emplace_back(Get<"HitID">(*hit)); };
    Get<"chi2">(*event) = minimum.Fval();
    Get<"t0">(*event) = t0;
    Get<"x0">(*event) = x0;
    Get<"theta">(*event) = d.theta();
    Get<"phi">(*event) = d.phi();
    return event;
}

} // namespace internal

auto LeastChiSquare(const std::vector<std::unique_ptr<LGAHit>>& eventHit) -> std::unique_ptr<CRMuEvent> {
    const auto& lga{Detector::Description::LGA::Instance()};
    struct HitForFit {
        Eigen::Vector2d x;
        Eigen::Matrix2d invCov;
        double z;
    };
    std::vector<HitForFit> hitForFit;
    hitForFit.reserve(eventHit.size());
    for (auto&& hit : eventHit) {
        const auto [varX, varY, covXY]{*Get<"covX">(*hit)};
        const Eigen::Matrix2d cov{
            {varX,  covXY},
            {covXY, varY }
        };
        hitForFit.push_back({Get<"x">(*hit).As<Eigen::Vector2d>(),
                             cov.inverse(),
                             lga.ModuleZ(Get<"ModID">(*hit))});
    }
    const ROOT::Math::WrappedMultiFunction ChiSquare{
        [&](const double* param) {
            const Eigen::Vector2d x0{param[0], param[1]};
            const auto d(Eigen::Vector3d{param[2], param[3], 1}.normalized());
            const Eigen::Vector2d d0{d.x(), d.y()};
            return muc::ranges::transform_reduce(
                hitForFit, 0., std::plus{},
                [&](auto&& hit) {
                    const Eigen::Vector2d delta{hit.x - (x0 + (hit.z / d.z()) * d0)};
                    return delta.dot(hit.invCov * delta);
                });
        }};
    return internal::LeastSquare(eventHit, ChiSquare);
}

auto LeastChiSquareSameWeight(const std::vector<std::unique_ptr<LGAHit>>& eventHit) -> std::unique_ptr<CRMuEvent> {
    const auto& lga{Detector::Description::LGA::Instance()};
    const auto variance{muc::pow<2>(lga.LGACellWidth()) / 12};
    struct HitForFit {
        Eigen::Vector2d x;
        double z;
    };
    std::vector<HitForFit> hitForFit;
    hitForFit.reserve(eventHit.size());
    for (auto&& hit : eventHit) {
        hitForFit.push_back({Get<"x">(*hit).As<Eigen::Vector2d>(),
                             lga.ModuleZ(Get<"ModID">(*hit))});
    }
    const ROOT::Math::WrappedMultiFunction ChiSquare{
        [&](const double* param) {
            const Eigen::Vector2d x0{param[0], param[1]};
            const auto d(Eigen::Vector3d{param[2], param[3], 1}.normalized());
            const Eigen::Vector2d d0{d.x(), d.y()};
            return muc::ranges::transform_reduce(
                hitForFit, 0., std::plus{},
                [&](auto&& hit) {
                    const Eigen::Vector2d delta{hit.x - (x0 + (hit.z / d.z()) * d0)};
                    return delta.dot(delta) / variance;
                });
        }};
    return internal::LeastSquare(eventHit, ChiSquare);
}

} // namespace CRMuReconstruction
} // namespace

auto ReconstructCRMu(const std::vector<std::unique_ptr<LGAHit>>& eventHit, std::string_view method) -> std::unique_ptr<CRMuEvent> {
    const auto eventID{*Get<"EvtID">(*eventHit.front())};
    if (std::ranges::any_of(eventHit, [&](auto&& h) { return Get<"EvtID">(*h) != eventID; })) {
        Mustard::Throw<std::runtime_error>("Event IDs in digi data are not all the same");
    }

    std::unique_ptr<CRMuEvent> event;
    if (method == "LeastChiSquare") {
        event = CRMuReconstruction::LeastChiSquare(eventHit);
    } else if (method == "LeastChiSquareSameWeight") {
        event = CRMuReconstruction::LeastChiSquareSameWeight(eventHit);
    } else {
        Mustard::Throw<std::runtime_error>(fmt::format("No method named '{}'", method));
    }
    if (event == nullptr) {
        return event;
    }

    Get<"EvtID">(*event) = eventID;
    return event;
}

} // namespace Musae::ReconLGA
