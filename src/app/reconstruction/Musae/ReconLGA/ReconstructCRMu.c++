#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/ReconstructCRMu.h++"

#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Geometry/Point3D.h"
#include "CLHEP/Geometry/Transform3D.h"
#include "CLHEP/Geometry/Vector3D.h"
#include "CLHEP/Vector/ThreeVector.h"

#include "Math/WrappedFunction.h"
#include "Minuit2/Minuit2Minimizer.h"

#include "Eigen/Dense"

#include "muc/math"
#include "muc/numeric"

#include "gsl/gsl"

#include <algorithm>
#include <functional>
#include <tuple>

namespace Musae::ReconLGA {

namespace {
namespace CRMuReconstruction {

template<bool ASameWeight>
auto LeastChiSquare(const muc::unique_ptrvec<LGAHit>& eventHit) -> std::unique_ptr<CRMuEvent> {
    const auto& lga{Detector::Description::LGA::Instance()};
    const auto nHit{gsl::narrow<int>(eventHit.size())};

    Eigen::VectorXd measurement;
    measurement.resize(2 * nHit);
    for (int i{}; i < nHit; ++i) {
        const auto [x, y]{*Get<"x">(*eventHit[i])};
        measurement[2 * i] = x;
        measurement[2 * i + 1] = y;
    }

    Eigen::MatrixXd covarianceInverse;
    covarianceInverse.resize(2 * nHit, 2 * nHit);
    if constexpr (ASameWeight) {
        covarianceInverse.setIdentity();
    } else {
        covarianceInverse.setZero();
        for (int i{}; i < nHit; ++i) {
            const auto [varX, varY, covXY]{*Get<"covX">(*eventHit[i])};
            covarianceInverse.block<2, 2>(2 * i, 2 * i) = Eigen::Matrix2d{
                {varX,  covXY},
                {covXY, varY }
            }.inverse();
        }
    }

    Eigen::MatrixX4d coefficient;
    coefficient.resize(2 * nHit, 4);
    for (int i{}; i < nHit; ++i) {
        coefficient.block<2, 2>(2 * i, 0) = Eigen::Matrix2d::Identity();
        coefficient.block<2, 2>(2 * i, 2) = lga.ModuleZ(Get<"ModID">(*eventHit[i])) * Eigen::Matrix2d::Identity();
    }

    const auto coeffTCovInv{(coefficient.transpose() * covarianceInverse).eval()};
    const auto param{((coeffTCovInv * coefficient).ldlt().solve(coeffTCovInv * measurement)).eval()};

    const auto deltaX{(coefficient * param - measurement).eval()};
    const auto chiSquare{deltaX.dot(covarianceInverse * deltaX)};
    const auto x0(lga.Rotation() * HepGeom::Point3D<double>{param[0], param[1], 0});
    const auto d(lga.Rotation() * HepGeom::Vector3D<double>{param[2], param[3], 1});

    const auto t0{muc::ranges::transform_reduce(
                      eventHit, 0., std::plus{},
                      [](auto&& h) {
                          return Get<"t">(*h) / muc::pow<2>(*Get<"sigmaT">(*h));
                      }) /
                  muc::ranges::transform_reduce(
                      eventHit, 0., std::plus{},
                      [](auto&& h) {
                          return 1 / muc::pow<2>(*Get<"sigmaT">(*h));
                      })};

    auto event{std::make_unique_for_overwrite<CRMuEvent>()};
    for (auto&& hit : eventHit) { Get<"HitID">(*event)->emplace_back(Get<"HitID">(*hit)); };
    Get<"chi2">(*event) = chiSquare;
    Get<"t0">(*event) = t0;
    Get<"x0">(*event) = static_cast<CLHEP::Hep3Vector>(x0);
    Get<"theta">(*event) = d.theta();
    Get<"phi">(*event) = d.phi();
    return event;
}

} // namespace CRMuReconstruction
} // namespace

auto ReconstructCRMu(const muc::unique_ptrvec<LGAHit>& eventHit, std::string_view method) -> std::unique_ptr<CRMuEvent> {
    const auto eventID{*Get<"EvtID">(*eventHit.front())};
    if (std::ranges::any_of(eventHit, [&](auto&& h) { return Get<"EvtID">(*h) != eventID; })) {
        Mustard::Throw<std::runtime_error>("Event IDs in digi data are not all the same");
    }

    std::unique_ptr<CRMuEvent> event;
    if (method == "LeastChiSquare") {
        event = CRMuReconstruction::LeastChiSquare<false>(eventHit);
    } else if (method == "LeastChiSquareSameWeight") {
        event = CRMuReconstruction::LeastChiSquare<true>(eventHit);
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
