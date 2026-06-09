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
#include "Musae/VisLGA/EventPlot.h++"

#include "Mustard/Utility/Print.h++"
#include "Mustard/Utility/VectorCast.h++"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

#include "TArrow.h"
#include "TDirectory.h"
#include "TEllipse.h"
#include "TFile.h"
#include "TH2F.h"
#include "TMarker.h"
#include "TPad.h"
#include "TStyle.h"

#include "Eigen/Dense"

#include "muc/hash_map"
#include "muc/numeric"

#include <iostream>
#include <limits>
#include <utility>

namespace Musae::VisLGA {

auto EventPlot(const muc::shared_ptrvec<LGADigi>& eventDigi,
               const muc::shared_ptrvec<LGAHit>& eventHit,
               const CRMuEvent* cRMuEvent) -> std::unique_ptr<TCanvas> {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    // make digi map

    LGADigiMap<const LGADigi*> eventDigiMap;
    for (auto&& digi : eventDigi) {
        const auto& ch{lga.TryChannelInfo(Get<"channelID">(*digi))};
        if (ch) {
            eventDigiMap[ch->moduleID][ch->edge].emplace_back(digi.get());
        }
    }

    // new canvas

    const auto eventID{*Get<"EvtID">(*eventHit.front())};
    const auto canvasName{fmt::format("Event{}", eventID)};
    const auto nHit{static_cast<int>(ssize(eventHit))};
    auto canvas{std::make_unique<TCanvas>(canvasName.c_str(), canvasName.c_str(), nHit * 450, 450)};
    canvas->cd();

    // add pads

    std::vector<TPad*> padList;
    for (auto&& hit : eventHit) {
        const auto hitID{*Get<"HitID">(*hit)};
        const auto padLeft{static_cast<double>(hitID) / nHit};
        const auto padRight{static_cast<double>(hitID + 1) / nHit};
        const auto padName{fmt::format("Pad{}", hitID)};
        auto& pad{padList.emplace_back(
            new TPad{padName.c_str(), padName.c_str(), padLeft, 0, padRight, 1})};
        pad->Draw();
    }
    auto pad{padList.begin()};

    const auto lgaWidthX{lga.NFiberX() * lga.LGACellWidth()};
    const auto lgaWidthY{lga.NFiberY() * lga.LGACellWidth()};

    // new track point (if any)

    muc::flat_hash_map<int, std::pair<TMarker*, TArrow*>> trackPoint;
    if (cRMuEvent) {
        const Eigen::Vector2d x0{Get<"x0">(*cRMuEvent)[0], Get<"x0">(*cRMuEvent)[1]};
        const auto sinTheta{std::sin(Get<"theta">(*cRMuEvent))};
        const Eigen::Vector3d d{sinTheta * std::cos(Get<"phi">(*cRMuEvent)),
                                sinTheta * std::sin(Get<"phi">(*cRMuEvent)),
                                std::cos(Get<"theta">(*cRMuEvent))};
        const Eigen::Vector2d d0{d.x(), d.y()};
        for (int i{}; i < lga.NModule(); ++i) {
            const Eigen::Vector2d x{x0 + (lga.ModuleZ(i) / d.z()) * d0};
            const auto point(new TMarker{x.x(), x.y(), kFullCross});
            const auto arrowLength{muc::midpoint(lga.LGAWidthX(), lga.LGAWidthY())};
            const Eigen::Vector2d x1{x - (arrowLength / 2) * d0};
            const Eigen::Vector2d x2{x + (arrowLength / 2) * d0};
            const auto arrow(new TArrow{x1.x(), x1.y(), x2.x(), x2.y(), 0.01});
            arrow->SetAngle(30);
            trackPoint[i] = {point, arrow};
        }
    }

    // plot everything

    for (auto&& hit : eventHit) {
        (*pad++)->cd();

        // draw digi histogram

        std::vector<TMarker*> marker;
        const auto hitID{*Get<"HitID">(*hit)};
        const auto moduleID{*Get<"ModID">(*hit)};
        const auto hist(new TH2F{fmt::format("Hit{}_module{}", eventID, hitID, moduleID).c_str(),
                                 fmt::format("Event {}, hit {}, on module {}", eventID, hitID, moduleID).c_str(),
                                 lga.NFiberX(), -lgaWidthX / 2, lgaWidthX / 2,
                                 lga.NFiberY(), -lgaWidthY / 2, lgaWidthY / 2});
        for (auto i{1}; i <= hist->GetNcells(); ++i) {
            hist->SetBinContent(i, std::numeric_limits<float>::denorm_min());
        }
        for (auto&& digi : eventDigiMap.at(moduleID).at('x')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{ch.edgePosition, -lgaWidthY / 2,
                                           Get<"Good">(*digi) ? kFullTriangleUp : kOpenTriangleUp});
            trigger->SetMarkerColor(kMagenta);
            marker.emplace_back(trigger);
            for (int j{}; j < lga.NFiberY(); ++j) {
                hist->Fill(ch.edgePosition, lga.FiberY(j), Get<"energy">(*digi));
            }
        }
        for (auto&& digi : eventDigiMap.at(moduleID).at('y')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{-lgaWidthX / 2, ch.edgePosition,
                                           Get<"Good">(*digi) ? kFullTriangleUp : kOpenTriangleUp});
            trigger->SetMarkerColor(kMagenta);
            marker.emplace_back(trigger);
            for (int i{}; i < lga.NFiberX(); ++i) {
                hist->Fill(lga.FiberX(i), ch.edgePosition, Get<"energy">(*digi));
            }
        }
        hist->SetStats(false);
        hist->GetXaxis()->SetTitle("x (mm)");
        hist->GetYaxis()->CenterTitle();
        hist->GetYaxis()->SetTitle("y (mm)");
        hist->GetYaxis()->CenterTitle();
        hist->GetYaxis()->SetTitleOffset(1.25);
        gStyle->SetPalette(kAvocado);
        gStyle->SetNumberContours(256);
        hist->Draw("COLZ");
        for (auto&& m : marker) {
            m->Draw("SAME");
        }

        // draw hit

        const auto hitPosition(new TMarker{Get<"x">(*hit)[0], Get<"x">(*hit)[1], kFullCircle});
        hitPosition->SetMarkerColor(kRed);
        hitPosition->Draw("SAME");

        const Eigen::Matrix2d hitPosCov{
            {Get<"covX">(*hit)[0], Get<"covX">(*hit)[2]},
            {Get<"covX">(*hit)[2], Get<"covX">(*hit)[1]}
        };
        const Eigen::SelfAdjointEigenSolver<Eigen::Matrix2d> hitPosCovEigenSystem{hitPosCov};
        const auto variancePrime{hitPosCovEigenSystem.eigenvalues()};
        const auto theta{Mustard::VectorCast<CLHEP::Hep2Vector>(hitPosCovEigenSystem.eigenvectors().col(0).eval()).phi()};
        const auto hitPositionError(new TEllipse{Get<"x">(*hit)[0], Get<"x">(*hit)[1],
                                                 std::sqrt(variancePrime[0]), std::sqrt(variancePrime[1]),
                                                 0, 360, theta / CLHEP::degree});
        hitPositionError->SetLineColor(kRed);
        hitPositionError->SetFillColorAlpha(kRed, 0.2);
        hitPositionError->Draw("SAME");

        // draw track point (if any)

        if (trackPoint.contains(moduleID)) {
            const auto [point, arrow]{trackPoint.at(moduleID)};
            point->SetMarkerColor(kCyan);
            point->Draw("SAME");
            arrow->SetLineColor(kCyan);
            arrow->SetFillColor(kCyan);
            arrow->Draw("|>,SAME");
        }
    }

    return canvas;
}

} // namespace Musae::VisLGA
