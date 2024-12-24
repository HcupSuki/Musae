#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconHit/Reconstruct.h++"
#include "Musae/ReconHit/Type.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDataFrame.hxx"
#include "TCanvas.h"
#include "TDirectory.h"
#include "TFile.h"
#include "TH2F.h"
#include "TMarker.h"
#include "TPad.h"
#include "TStyle.h"

#include "muc/ceta_string"
#include "muc/math"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace Musae::ReconHit;

// coincident digi -> {event hit, good digi}
auto ProcessCoincidentDigi(const LGADigiMap<std::unique_ptr<const LGADigi>>& coincidentDigi,
                           int eventID, std::string_view method)
    -> std::pair<std::vector<std::unique_ptr<const LGAHit>>, LGADigiMap<const LGADigi*>>;

// plot hit of an event
auto PlotEvent(const LGADigiMap<std::unique_ptr<const LGADigi>>& coincidentDigi,
               const std::vector<std::unique_ptr<const LGAHit>>& eventHit,
               const LGADigiMap<const LGADigi*>& eventDigi)
    -> void;

auto main(int argc, char* argv[]) -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("input").help("Input file path(s).").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--input-tree").help("Input tree name.").required().nargs(1).default_value("data");
    cli->add_argument("-o", "--output").help("Output file path.").required().nargs(1);
    cli->add_argument("--output-mode").help("Output file creation mode.").required().nargs(1).default_value("NEW");
    cli->add_argument("-r", "--output-tree").help("Output tree name.").required().nargs(1).default_value("LGAHit");
    cli->add_argument("-p", "--plot-hit").help("Produce hit plots for an event range (e.g. -p <first> <last>).").scan<'i', int>().nargs(2);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    ROOT::RDataFrame data{cli->get("--input-tree"), cli->get<std::vector<std::string>>("input")};
    ROOT::RDF::Experimental::AddProgressBar(data);

    TFile file{cli->get("--output").c_str(), cli->get("--output-mode").c_str()};
    if (not file.IsOpen()) {
        return EXIT_FAILURE;
    }
    Mustard::Data::Output<Musae::Data::LGAHit> lgaHitOutput{cli->get("--output-tree"), "Reconstructed LGA hit data"};

    const LGADigi* headDigi{};
    LGADigiMap<std::unique_ptr<const LGADigi>> coincidentDigi; // {moduleID, edge} -> [digi...]
    int eventID{};
    const auto displayEventRange{cli->present<std::vector<int>>("-p")};
    data.Foreach(
        [&, &lga = Musae::Detector::Description::LGA::Instance()](
            Long64_t time, unsigned channelID, float energy) {
            // insert coincidence hit
            if (headDigi == nullptr or
                std::abs(time - Get<"time">(*headDigi)) * CLHEP::ps < lga.CoincidenceTimeWindow()) {
                // get module ID and "which edge"
                int moduleID;
                char edge;
                try {
                    const auto& ch{lga.ChannelInfo(channelID)};
                    moduleID = ch.moduleID;
                    edge = ch.edge;
                } catch (const std::out_of_range&) {
                    return;
                }
                // make and insert a digi
                auto digi{std::make_unique<const LGADigi>(time, channelID, energy)};
                if (headDigi == nullptr) {
                    headDigi = digi.get();
                }
                coincidentDigi[moduleID][edge].emplace_back(std::move(digi));
                return;
            }
            // process coincidence hit
            const auto [eventHit, eventDigi]{ProcessCoincidentDigi(coincidentDigi, eventID, "Weighted2D")};
            if (not eventHit.empty()) {
                lgaHitOutput.Fill(eventHit);
                if (displayEventRange.has_value() and
                    displayEventRange->front() <= eventID and eventID < displayEventRange->back()) {
                    PlotEvent(coincidentDigi, eventHit, eventDigi);
                }
                ++eventID;
            }
            // reset coincidence hit
            headDigi = nullptr;
            coincidentDigi.clear();
        },
        {"time", "channelID", "energy"});

    lgaHitOutput.Write();

    return EXIT_SUCCESS;
}

auto ProcessCoincidentDigi(const LGADigiMap<std::unique_ptr<const LGADigi>>& coincidentDigi,
                           int eventID, std::string_view method)
    -> std::pair<std::vector<std::unique_ptr<const LGAHit>>, LGADigiMap<const LGADigi*>> {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    std::pair<std::vector<std::unique_ptr<const LGAHit>>, LGADigiMap<const LGADigi*>> result;
    auto& [eventHit, eventDigi]{result};
    eventHit.reserve(lga.NModule());
    eventDigi.reserve(lga.NModule());

    // digi selection loop

    int hitID{};
    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{coincidentDigi.find(moduleID)};
        if (iDigiOfTheModule == coincidentDigi.cend()) {
            continue;
        }
        const auto& digiOfTheModule{iDigiOfTheModule->second};

        for (auto edge : {'x', 'y'}) {
            const auto iDigiOfTheEdge{digiOfTheModule.find(edge)};
            if (iDigiOfTheEdge == digiOfTheModule.cend()) {
                break;
            }
            const auto& digiOfTheEdge{iDigiOfTheEdge->second};
            // digi selection here
            for (auto&& digi : digiOfTheEdge) {
                if (Get<"energy">(*digi) > lga.LuminousDigiEnergyThreshold()) {
                    eventDigi[moduleID][edge].emplace_back(digi.get());
                }
            }
        }
    }

    // hit reconstruction loop

    for (auto moduleID : std::views::iota(0, lga.NModule())) {
        const auto iDigiOfTheModule{eventDigi.find(moduleID)};
        if (iDigiOfTheModule == eventDigi.cend()) {
            continue;
        }
        auto& digiOfTheModule{iDigiOfTheModule->second};

        if (not digiOfTheModule.contains('x') or
            not digiOfTheModule.contains('y')) {
            digiOfTheModule.clear();
            continue;
        }

        auto hit{Musae::ReconHit::Reconstruct(digiOfTheModule, eventID, hitID, method)};
        if (hit == nullptr) {
            digiOfTheModule.clear();
            continue;
        }

        eventHit.emplace_back(std::move(hit));
        ++hitID;
    }

    // n hit selection
    if (ssize(eventHit) < lga.NHitThreshold()) {
        return {};
    }

    return result;
}

auto PlotEvent(const LGADigiMap<std::unique_ptr<const LGADigi>>& coincidentDigi,
               const std::vector<std::unique_ptr<const LGAHit>>& eventHit,
               const LGADigiMap<const LGADigi*>& eventDigi) -> void {
    const auto pwd{TDirectory::CurrentDirectory().load()->GetPath()};
    gFile->cd(gFile->mkdir("HitPlot", "", true)->GetPath());

    const auto eventID{*Get<"EvtID">(*eventHit.front())};
    const auto canvasName{fmt::format("Event{}", eventID)};
    const auto nHit{static_cast<int>(ssize(eventHit))};
    TCanvas canvas{canvasName.c_str(), canvasName.c_str(), nHit * 450, 450};

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
    const auto lgaWidthX{lga.NLGACellX() * lga.LGACellWidth()};
    const auto lgaWidthY{lga.NLGACellY() * lga.LGACellWidth()};

    for (auto&& hit : eventHit) {
        pad++->cd();

        std::vector<TMarker*> marker;
        const auto hitID{*Get<"HitID">(*hit)};
        const auto moduleID{*Get<"ModID">(*hit)};
        const auto hist(new TH2F{fmt::format("Hit{}_module{}", eventID, hitID, moduleID).c_str(),
                                 fmt::format("Event {}, hit {}, on module {}", eventID, hitID, moduleID).c_str(),
                                 lga.NLGACellX(), -lgaWidthX / 2, lgaWidthX / 2,
                                 lga.NLGACellY(), -lgaWidthY / 2, lgaWidthY / 2});
        for (auto i{1}; i <= hist->GetNcells(); ++i) {
            hist->SetBinContent(i, std::numeric_limits<float>::denorm_min());
        }
        for (auto&& digi : coincidentDigi.at(moduleID).at('x')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{ch.edgePosition, -lgaWidthY / 2, kOpenTriangleDown});
            trigger->SetMarkerColor(kMagenta);
            trigger->SetMarkerSize(1.5);
            marker.emplace_back(trigger);
            for (int j{}; j < lga.NLGACellY(); ++j) {
                hist->Fill(ch.edgePosition, lga.FiberY(j), Get<"energy">(*digi));
            }
        }
        for (auto&& digi : coincidentDigi.at(moduleID).at('y')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto trigger(new TMarker{-lgaWidthX / 2, ch.edgePosition, kOpenTriangleDown});
            trigger->SetMarkerColor(kMagenta);
            trigger->SetMarkerSize(1.5);
            marker.emplace_back(trigger);
            for (int i{}; i < lga.NLGACellX(); ++i) {
                hist->Fill(lga.FiberX(i), ch.edgePosition, Get<"energy">(*digi));
            }
        }
        for (auto&& digi : eventDigi.at(moduleID).at('x')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto selected(new TMarker{ch.edgePosition, -lgaWidthY / 2, kFullTriangleDown});
            selected->SetMarkerColor(kMagenta);
            selected->SetMarkerSize(1.5);
            marker.emplace_back(selected);
        }
        for (auto&& digi : eventDigi.at(moduleID).at('y')) {
            const auto& ch{lga.ChannelInfo(Get<"channelID">(*digi))};
            const auto selected(new TMarker{-lgaWidthX / 2, ch.edgePosition, kFullTriangleDown});
            selected->SetMarkerColor(kMagenta);
            selected->SetMarkerSize(1.5);
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

        const auto x{Get<"x">(*hit)};
        const auto hitPosition(new TMarker{x[0], x[1], kFullCircle});
        hitPosition->SetMarkerColor(kRed);
        hitPosition->SetMarkerSize(1.5);
        hitPosition->Draw("SAME");
    }

    canvas.Write();

    gFile->cd(pwd);
}
