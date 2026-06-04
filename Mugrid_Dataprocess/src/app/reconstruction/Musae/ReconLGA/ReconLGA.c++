#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/FormatChannelSummary.h++"
#include "Musae/ReconLGA/ReconLGA.h++"
#include "Musae/ReconLGA/ReconstructAllHit.h++"
#include "Musae/ReconLGA/ReconstructCRMu.h++"
#include "Musae/ReconLGA/PlaneEfficiencyTest.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"
#include "Musae/ReconLGA/Type.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Data/SeqProcessor.h++"
#include "Mustard/Detector/Description/DescriptionIO.h++"
#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"
#include "Mustard/Utility/MakeTextTMacro.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/Print.h++"
#include "Mustard/Utility/ProgressBar.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TMacro.h"
#include "TNamed.h"
#include "TApplication.h"
#include "TH1.h"

#include "muc/hash_map"

#include "fmt/core.h"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Musae::ReconLGA {

ReconLGA::ReconLGA() :
    Subprogram{"ReconLGA", "LGA event reconstruction program."} {}

using namespace std::literals;

auto ReconLGA::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("input").help("Input file path(s).").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--input-tree").help("Input tree name.").default_value("data"s).required().nargs(1);
    cli->add_argument("-n", "--input-range").help("Input entry range.").nargs(2).scan<'i', unsigned>();
    cli->add_argument("-o", "--output").help("Output file path.").required().nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").default_value("NEW"s).required().nargs(1);
    cli->add_argument("-s", "--output-digi-summary").help("Output digi summary name.").default_value("LGADigiSummary"s).required().nargs(1);
    cli->add_argument("-g", "--output-digi-tree").help("Output digi tree name.").default_value("LGADigi"s).required().nargs(1);
    cli->add_argument("-h", "--output-hit-tree").help("Output hit tree name.").default_value("LGAHit"s).required().nargs(1);
    cli->add_argument("-e", "--output-event-tree").help("Output event tree name.").default_value("CRMuEvent"s).required().nargs(1);
    cli->add_argument("-r", "--hit-method").help("Hit reconstruction method.").default_value("SLinTW"s).required().nargs(1);
    cli->add_argument("-k", "--no-crmu").help("Skip cosmic-ray muon event reconstruction.").flag();
    cli->add_argument("-e", "--plane-efficiency-test").help("Test plane efficiency of a three layer detector.").flag();
    cli->add_argument("-u", "--crmu-method").help("Cosmic-ray muon event reconstruction method.").default_value("LeastChiSquare"s).required().nargs(1);
    cli->add_argument("-c", "--lga-description").help("LGA description YAML file path.").nargs(1);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    const auto hitReconstructionMethod{ParseReconstructHitMethod(cli->get("--hit-method"))};
    const auto reconstructCRMu{cli["--no-crmu"] == false};
    const auto PlaneEfficiencyTest{cli["--plane-efficiency-test"] == true};
    const auto cRMuReconstructionMethod{ParseReconstructCRMuMethod(cli->get("--crmu-method"))};

    ROOT::RDF::RNode data(ROOT::RDataFrame{cli->get("--input-tree"), cli->get<std::vector<std::string>>("input")});
    if (const auto entryRange{cli->present<std::vector<unsigned>>("--input-range")}) {
        data = data.Range(entryRange->front(), entryRange->back());
    }

    TFile file{cli->get("--output").c_str(), cli->get("--output-mode").c_str()};
    if (not file.IsOpen()) {
        return EXIT_FAILURE;
    }

    if (const auto lgaDescriptionPath{cli->present("--lga-description")}) {
        Mustard::Detector::Description::DescriptionIO::
            Import<Musae::Detector::Description::LGA>(*lgaDescriptionPath);
    }
    Mustard::MakeTextTMacro(Mustard::Detector::Description::DescriptionIO::
                                ToString<Musae::Detector::Description::LGA>(),
                            "LGA")
        ->Write();
    const auto& lga{Musae::Detector::Description::LGA::Instance()};
    // digi data summary

    muc::flat_hash_map<int, ChannelSummary> flatChannelSummary;
    Mustard::ProgressBar progressBar;
    const auto SummarizeDigi{[&](unsigned channelID, float energy) {
        auto& ch{flatChannelSummary[channelID]};
        ch.channelID = channelID;
        ch.meanEnergy += energy;
        ++ch.triggerCount;
        progressBar.Tick();
    }};

    Mustard::PrintLn("Summarizing LGA digi data...");
    progressBar.Start(*data.Count());
    data.Foreach(SummarizeDigi, {"channelID", "energy"});
    progressBar.Complete();
    for (auto&& [_, ch] : flatChannelSummary) {
        ch.meanEnergy /= ch.triggerCount;
    }
    Mustard::PrintLn("Summarization completed.");

    const auto channelSummaryText{FormatChannelSummary(flatChannelSummary)};
    Mustard::Print<'W'>("{}", channelSummaryText);
    Mustard::MakeTextTMacro(channelSummaryText, cli->get("--output-digi-summary"))->Write();

    // reconstruction function

    std::optional<long long> triggerTime{};
    std::optional<long long> eventCloseTime{};
    std::optional<long long> lastDigiTime{};
    std::array<std::optional<TH2D>, 3> hitatleast2plane;
    std::array<std::optional<TH2D>, 3> hitAllplane;
    std::array<std::optional<TH2D>, 3> efficiencyHist;
    const int binnum{30};
    const auto coincidenceTimeWindow{std::llround(lga.CoincidenceTimeWindow() / CLHEP::ps)};
    const auto softDeadTime{std::llround(lga.SoftDeadTime() / CLHEP::ps)};
    int NHitThresholdCount{};
    int eventID{};
    long long int AllHitCount{};
    std::vector<long long int> IgnoreOneModCount(lga.NModule()); // hit count ignore one specfic module
    long long UpEventCount{};   
    LGADigiMap<std::unique_ptr<LGADigi>> eventDigi; // {moduleID, edge} -> [digi...]
    Mustard::Data::Output<Musae::Data::LGADigi> lgaDigiOutput{cli->get("--output-digi-tree"), "Reconstructed LGA hit digi"};
    Mustard::Data::Output<Musae::Data::LGAHit> lgaHitOutput{cli->get("--output-hit-tree"), "Reconstructed LGA hit data"};
    Mustard::Data::Output<Musae::Data::CRMuEvent> cRMuEventOutput{cli->get("--output-event-tree"), "Reconstructed cosmic-ray muon event"};

    if (PlaneEfficiencyTest) {
        const auto xRange = lga.LGAWidthX() / 2.0;
        const auto yRange = lga.LGAWidthY() / 2.0;
        
        for (int i = 0; i < 3; ++i) {
            hitatleast2plane[i].emplace(
                fmt::format("hitatleast2plane{}", i).c_str(), "HitAtLeast2Plane",
                binnum, -xRange, xRange, binnum, -yRange, yRange
            );
            hitAllplane[i].emplace(
                fmt::format("hitAllplane{}", i).c_str(), "HitAllPlane",
                binnum, -xRange, xRange, binnum, -yRange, yRange
            );
            efficiencyHist[i].emplace(
                fmt::format("efficiencyHist{}", i).c_str(), "EfficiencyHist",
                binnum, -xRange, xRange, binnum, -yRange, yRange
            );
            efficiencyHist[i]->GetXaxis()->SetTitle("X [mm]");
            efficiencyHist[i]->GetYaxis()->SetTitle("Y [mm]");
        }
        
    }
    
    // const auto ProcessDigi{[&](std::shared_ptr<LGARawDigi> digi) {
    //     // insert coincidence hit
    //     if (eventCloseTime and std::abs(Get<"time">(*digi) - *eventCloseTime) < softDeadTime) {
    //         return;
    //     }
    //     if (triggerTime == std::nullopt) {
    //         triggerTime = Get<"time">(*digi);
    //         eventCloseTime = std::nullopt;
    //     }
    //     // triggerTime = Get<"time">(*digi);
        
    //     const auto t{Get<"time">(*digi) - *triggerTime};
    //     if (std::abs(t) < coincidenceTimeWindow) {
    //         const auto ch{lga.TryChannelInfo(Get<"channelID">(*digi))};
    //         if (ch == nullptr) { return; }
    //         // make and insert a digi
    //         const auto normalizedEnergy{Get<"energy">(*digi) / flatChannelSummary.at(Get<"channelID">(*digi)).meanEnergy};
    //         eventDigi[ch->moduleID][ch->edge].emplace_back(
    //             std::make_unique<LGADigi>(Get<"time">(*digi), Get<"channelID">(*digi), Get<"energy">(*digi),
    //                                       eventID, *triggerTime, t * CLHEP::ps, false,
    //                                       ch->moduleID, ch->edge, ch->fiberLocalID,
    //                                       normalizedEnergy));
    //         return;
    //     }
        

    //     // process coincidence digi
    //     [&] {
    //         // reconstruct hits
    //         const auto eventHit{ReconstructAllHit(eventDigi, hitReconstructionMethod)};
    //         if (eventHit.empty()) { return; }
    //         for (auto&& [_, moduleDigi] : std::as_const(eventDigi)) {
    //             for (auto&& [_, digi] : std::as_const(moduleDigi)) {
    //                 lgaDigiOutput.Fill(digi);
    //             }
    //         }
    //         lgaHitOutput.Fill(eventHit);
    //         // reconstruct cosmic-ray muon event
    //         std::unique_ptr<CRMuEvent> cRMuEvent;
    //         if (reconstructCRMu) {
    //             cRMuEvent = ReconstructCRMu(eventHit, cRMuReconstructionMethod);
    //             if (cRMuEvent) {
    //                 cRMuEventOutput.Fill(*cRMuEvent);
    //             } else {
    //                 Mustard::PrintWarning(fmt::format("Failed to reconstruct event {}", eventID));
    //             }
    //         }
    //         eventCloseTime = Get<"time">(*digi);
    //         ++eventID;
    //     }();

    //     // reset coincidence hit
    //     triggerTime = std::nullopt;
    //     eventDigi.clear();
    // }};

    const auto ProcessDigiDynamic{[&](std::shared_ptr<LGARawDigi> digi) {
        // insert coincidence hit
        if (eventCloseTime and std::abs(Get<"time">(*digi) - *eventCloseTime) < softDeadTime) {
            return;
        }
        if (triggerTime == std::nullopt) {
            triggerTime = Get<"time">(*digi);
            lastDigiTime = triggerTime;
            eventCloseTime = std::nullopt;
        }

        if (std::abs(Get<"time">(*digi) - *lastDigiTime) < coincidenceTimeWindow) {
            const auto ch{lga.TryChannelInfo(Get<"channelID">(*digi))};
            if (ch == nullptr) { return; }
            // make and insert a digi
            const auto normalizedEnergy{Get<"energy">(*digi) / flatChannelSummary.at(Get<"channelID">(*digi)).meanEnergy};
            eventDigi[ch->moduleID][ch->edge].emplace_back(
                std::make_unique<LGADigi>(Get<"time">(*digi), Get<"channelID">(*digi), Get<"energy">(*digi),
                                          eventID, *triggerTime, (Get<"time">(*digi) - *triggerTime) * CLHEP::ps, false,
                                          ch->moduleID, ch->edge, ch->fiberLocalID,
                                          normalizedEnergy));
            lastDigiTime = Get<"time">(*digi);
            return;
        }
        // lastDigiTime = Get<"time">(*digi);
        // triggerTime = Get<"time">(*digi);
        

        // process coincidence digi
        [&] {
            // reconstruct hits
            const auto eventHit{ReconstructAllHit(eventDigi, hitReconstructionMethod, UpEventCount)};
            if (eventHit.empty()) { return; }
            lgaHitOutput.Fill(eventHit);
            if (ssize(eventHit) == lga.NModule())
            {
                AllHitCount++;
                std::for_each(IgnoreOneModCount.begin(), IgnoreOneModCount.end(), [](long long int& val) {
                    val += 1;
                });
            }
            else if (ssize(eventHit) == lga.NModule() - 1)
            {
                // find which module is ignored
                std::vector<bool> moduleHitFlag(lga.NModule(), false);
                for (auto&& hit : eventHit)
                {
                    moduleHitFlag[Get<"ModID">(*hit)] = true;
                }
                for (size_t modID = 0; modID < moduleHitFlag.size(); ++modID)
                {
                    if (!moduleHitFlag[modID])
                    {
                        IgnoreOneModCount[modID]++;
                        break;
                    }
                }
            }
            if (ssize(eventHit) < lga.NHitThreshold()) {
                ++eventID;
                return;
            }
            else{
                for (auto&& [_, moduleDigi] : std::as_const(eventDigi)) {
                    for (auto&& [_, digi] : std::as_const(moduleDigi)) {
                        lgaDigiOutput.Fill(digi);
                    }
                }
            // lgaHitOutput.Fill(eventHit);
            // reconstruct cosmic-ray muon event
                std::unique_ptr<CRMuEvent> cRMuEvent;
                if (reconstructCRMu) {
                    cRMuEvent = ReconstructCRMu(eventHit, cRMuReconstructionMethod);
                    if (cRMuEvent) {
                        cRMuEventOutput.Fill(*cRMuEvent);
                    } else {
                        Mustard::PrintWarning(fmt::format("Failed to reconstruct event {}", eventID));
                    }
                }
                if (PlaneEfficiencyTest) {
                    const auto xRange = lga.LGAWidthX() / 2.0;
                    const auto yRange = lga.LGAWidthY() / 2.0;
                    for (int i = 0; i < 3; ++i) {
                        PlaneEfficiencyCal(eventHit, *hitatleast2plane[i], *hitAllplane[i], i, xRange, yRange);
                    }
                }
            }
            eventCloseTime = Get<"time">(*digi);
            ++NHitThresholdCount;
            ++eventID;
        }();

        // reset coincidence hit
        triggerTime = std::nullopt;
        eventDigi.clear();
    }};

    // main reconstruction loop

    Mustard::Data::SeqProcessor processor;

    Mustard::PrintLn("Reconstructing events...");
    processor.Process<Musae::Data::LGARawDigi>(data, ProcessDigiDynamic);
    // processor.Process<Musae::Data::LGARawDigi>(data, ProcessDigi);
    Mustard::PrintLn("Reconstruction completed.");

    const auto totalSoftDeadTime{NHitThresholdCount * (softDeadTime * CLHEP::ps)};
    TNamed{fmt::format("Soft dead time: {:.1f} s", totalSoftDeadTime / CLHEP::s),
           fmt::format("Total soft dead time: {} s, accumulated from {} NHitThresholdCount' {}-ms dead time",
                       totalSoftDeadTime / CLHEP::s, NHitThresholdCount, softDeadTime / CLHEP::ms)}
        .Write();
    for (int modID = 0; modID < lga.NModule(); ++modID)
    {
        TNamed{fmt::format("Efficiency of mod {}: {}", modID, static_cast<double>(AllHitCount)/IgnoreOneModCount[modID]),
               fmt::format("AllHitCount: {}, IgnoreOneModCount: {}", AllHitCount, IgnoreOneModCount[modID])}
            .Write();
    }
    std::cout << "UpEventCount: " << UpEventCount << std::endl;
    lgaDigiOutput.Write();
    lgaHitOutput.Write();
    if (reconstructCRMu) {
        cRMuEventOutput.Write();
    }
    if (PlaneEfficiencyTest) {
        for (int i = 0; i < 3; ++i) {
            efficiencyHist[i]->Divide(&*hitAllplane[i], &*hitatleast2plane[i], 1, 1, "B");
            efficiencyHist[i]->Write();
        }
        // hitatleast2plane0->Write();
        // hitAllplane0->Write();
    }
    return EXIT_SUCCESS;
}

} // namespace Musae::ReconLGA
