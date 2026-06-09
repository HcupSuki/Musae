// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/PlaneEfficiencyTest.h++"

#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/LiteralUnit.h++"

#include "G4SystemOfUnits.hh"

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
namespace LinearFit {
    
auto LeastChiSquare(const muc::unique_ptrvec<LGAHit>& eventHit, int planeid) -> std::tuple<double, double> {
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
    covarianceInverse.resize(2 * nHit, 2 * nHit); // inverse of covariance matrix
    covarianceInverse.setZero();
    for (int i{}; i < nHit; ++i) {
        const auto [varX, varY, covXY]{*Get<"covX">(*eventHit[i])};
        covarianceInverse.block<2, 2>(2 * i, 2 * i) = Eigen::Matrix2d{
            {varX,  covXY},
            {covXY, varY }
        }.inverse(); // compute the inverse of covariance matrix
    }

    Eigen::MatrixX4d coefficient;
    coefficient.resize(2 * nHit, 4);
    for (int i{}; i < nHit; ++i) {
        coefficient.block<2, 2>(2 * i, 0) = Eigen::Matrix2d::Identity();
        coefficient.block<2, 2>(2 * i, 2) = lga.ModuleZ(Get<"ModID">(*eventHit[i])) * Eigen::Matrix2d::Identity();
    }

    const auto coeffTCovInv{(coefficient.transpose() * covarianceInverse).eval()};
    const auto param{((coeffTCovInv * coefficient).ldlt().solve(coeffTCovInv * measurement)).eval()};

    const auto x0(lga.Rotation() * HepGeom::Point3D<double>{param[0], param[1], 0});
    const auto d(lga.Rotation() * HepGeom::Vector3D<double>{param[2], param[3], 1});

    auto x = x0.x() + d.x() * (lga.ModuleZ(planeid) - x0.z()) / d.z();
    auto y = x0.y() + d.y() * (lga.ModuleZ(planeid) - x0.z()) / d.z();
    return std::make_tuple(x, y);
}

} // namespace LinearFit
} // namespace

auto PlaneEfficiencyCal(const muc::unique_ptrvec<LGAHit>& eventHit, TH2D& hitatleast2plane, TH2D& hitAllplane, int planeid, double xRange, double yRange) -> void {
    static constexpr std::array<std::pair<int, int>, 3> planeConfig{{
        {2, 1},  // planeid 0
        {2, 0},  // planeid 1
        {1, 0}   // planeid 2
    }};
    if (planeid < 0 || planeid > 2) {
        Mustard::PrintError("Plane ID must be 0, 1 or 2.");
        return;
    }
    const auto [upplane, downplane] = planeConfig[planeid];
    const auto nHit{gsl::narrow<int>(eventHit.size())};
    if (nHit == 3)
    {
        const auto [x, y]{*Get<"x">(*eventHit[planeid])};
        hitatleast2plane.Fill(x, y);
        hitAllplane.Fill(x, y);
    }
    else if (nHit == 2)
    {
        const auto mod0 = Get<"ModID">(*eventHit[0]);
        const auto mod1 = Get<"ModID">(*eventHit[1]);
        
        if ((mod0 == upplane && mod1 == downplane) || 
            (mod0 == downplane && mod1 == upplane)) {
            auto [x, y] = LinearFit::LeastChiSquare(eventHit, planeid);
            if (std::abs(x) > xRange || std::abs(y) > yRange) {
                return;
            }
            hitatleast2plane.Fill(x, y);
        }
    }
}

} // namespace Musae::ReconLGA
