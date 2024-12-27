#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/PlotEvent.h++"

#include "Mustard/Utility/VectorCast.h++"

#include "CLHEP/Units/SystemOfUnits.h"
#include "CLHEP/Vector/TwoVector.h"

#include "TArrow.h"
#include "TCanvas.h"
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

#include <limits>
#include <list>
#include <utility>

namespace Musae::ReconLGA {

auto PlotEvent(const LGADigiMap<std::unique_ptr<LGADigi>>& coincidentDigi,
               const LGADigiMap<LGADigi*>& eventDigi,
               const std::vector<std::unique_ptr<LGAHit>>& eventHit,
               const CRMuEvent* crMuEvent) -> void {
    const auto pwd{TDirectory::CurrentDirectory().load()->GetPath()};
    gFile->cd(gFile->mkdir("LGAHitPlot", "", true)->GetPath());

    // new canvas

    const auto eventID{*Get<"EvtID">(*eventHit.front())};
    const auto canvasName{fmt::format("Event{}", eventID)};
    const auto nHit{static_cast<int>(ssize(eventHit))};
    TCanvas canvas{canvasName.c_str(), canvasName.c_str(), nHit * 450, 450};

    // add pads

    std::list<TPad> padList;
    for (auto&& hit : eventHit) {
        const auto hitID{*Get<"HitID">(*hit)};
        const auto padLeft{static_cast<double>(hitID) / nHit};
        const auto padRight{static_cast<double>(hitID + 1) / nHit};
        const auto padName{fmt::format("Pad{}", hitID)};
        auto& pad{padList.emplace_back(padName.c_str(), padName.c_str(), padLeft, 0, padRight, 1)};
        pad.Draw();
    }
    auto pad{padList.begin()};

    const auto& lga{Musae::Detector::Description::LGA::Instance()};
    const auto lgaWidthX{lga.NFiberX() * lga.LGACellWidth()};
    const auto lgaWidthY{lga.NFiberY() * lga.LGACellWidth()};

    // new track point (if any)

    muc::flat_hash_map<int, std::pair<TMarker*, TArrow*>> trackPoint;
    if (crMuEvent) {
        const Eigen::Vector2d x0{Get<"x0">(*crMuEvent)[0], Get<"x0">(*crMuEvent)[1]};
        const auto sinTheta{std::sin(Get<"theta">(*crMuEvent))};
        const Eigen::Vector3d d{sinTheta * std::cos(Get<"phi">(*crMuEvent)),
                                sinTheta * std::sin(Get<"phi">(*crMuEvent)),
                                std::cos(Get<"theta">(*crMuEvent))};
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
        pad++->cd();

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
        for (auto&& digi : coincidentDigi.at(moduleID).at('x')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{ch.edgePosition, -lgaWidthY / 2, kOpenTriangleUp});
            trigger->SetMarkerColor(kMagenta);
            marker.emplace_back(trigger);
            for (int j{}; j < lga.NFiberY(); ++j) {
                hist->Fill(ch.edgePosition, lga.FiberY(j), Get<"energy">(*digi));
            }
        }
        for (auto&& digi : coincidentDigi.at(moduleID).at('y')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{-lgaWidthX / 2, ch.edgePosition, kOpenTriangleUp});
            trigger->SetMarkerColor(kMagenta);
            marker.emplace_back(trigger);
            for (int i{}; i < lga.NFiberX(); ++i) {
                hist->Fill(lga.FiberX(i), ch.edgePosition, Get<"energy">(*digi));
            }
        }
        for (auto&& digi : eventDigi.at(moduleID).at('x')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto selected(new TMarker{ch.edgePosition, -lgaWidthY / 2, kFullTriangleUp});
            selected->SetMarkerColor(kMagenta);
            marker.emplace_back(selected);
        }
        for (auto&& digi : eventDigi.at(moduleID).at('y')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto selected(new TMarker{-lgaWidthX / 2, ch.edgePosition, kFullTriangleUp});
            selected->SetMarkerColor(kMagenta);
            marker.emplace_back(selected);
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

    canvas.Write();

    gFile->cd(pwd);
}

} // namespace Musae::ReconLGA
