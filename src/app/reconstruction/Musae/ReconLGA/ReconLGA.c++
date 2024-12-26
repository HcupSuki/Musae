#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/FormatChannelSummary.h++"
#include "Musae/ReconLGA/PlotEvent.h++"
#include "Musae/ReconLGA/ReconstructAllHit.h++"
#include "Musae/ReconLGA/ReconstructCRMu.h++"
#include "Musae/ReconLGA/ReconstructHit.h++"
#include "Musae/ReconLGA/Type.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"
#include "Mustard/Utility/MakeTextTMacro.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/Print.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TMacro.h"

#include "muc/hash_map"

#include "fmt/core.h"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace Musae::ReconLGA;
using namespace std::string_literals;

auto main(int argc, char* argv[]) -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("input").help("Input file path(s).").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--input-tree").help("Input tree name.").required().nargs(1).default_value("data"s);
    cli->add_argument("-n", "--input-range").help("Input entry range.").nargs(2).scan<'i', unsigned>();
    cli->add_argument("-o", "--output").help("Output file path.").required().nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").required().nargs(1).default_value("NEW"s);
    cli->add_argument("-s", "--output-digi-summary").help("Output digi summary name.").required().nargs(1).default_value("LGADigiSummary"s);
    cli->add_argument("-h", "--output-hit-tree").help("Output hit tree name.").required().nargs(1).default_value("LGAHit"s);
    cli->add_argument("-e", "--output-event-tree").help("Output event tree name.").required().nargs(1).default_value("CRMuEvent"s);
    cli->add_argument("-p", "--plot-hit").help("Produce hit plots for an event range (e.g. -p <first> <last>).").nargs(2).scan<'i', int>();
    cli->add_argument("-z", "--daq-t0").help("DAQ start time (in second).").required().nargs(1).scan<'g', double>().default_value(0.);
    cli->add_argument("-r", "--hit-method").help("Hit reconstruction method.").required().nargs(1).default_value("Weighted2D"s);
    cli->add_argument("-c", "--no-crmu").help("Skip cosmic-ray muon event reconstruction.").flag();
    cli->add_argument("-r", "--crmu-method").help("Cosmic-ray muon event reconstruction method.").required().nargs(1).default_value("LeastChiSquare"s);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    const auto plotEventRange{cli->present<std::vector<int>>("--plot-hit")};
    const auto hitReconstructionMethod{cli->get("--hit-method")};
    const auto reconstructCRMu{cli["--no-crmu"] == false};
    const auto crMuReconstructionMethod{cli->get("--crmu-method")};

    ROOT::RDF::RNode data(ROOT::RDataFrame{cli->get("--input-tree"), cli->get<std::vector<std::string>>("input")});
    if (const auto entryRange{cli->present<std::vector<unsigned>>("--input-range")}) {
        data = data.Range(entryRange->front(), entryRange->back());
    }

    TFile file{cli->get("--output").c_str(), cli->get("--output-mode").c_str()};
    if (not file.IsOpen()) {
        return EXIT_FAILURE;
    }

    const auto& lga{Musae::Detector::Description::LGA::Instance()};

    // digi data summary

    Long64_t daqTimePicoseconds{std::numeric_limits<Long64_t>::lowest()};
    muc::flat_hash_map<int, ChannelSummary> flatChannelSummary;
    Mustard::PrintLn("Summarizing LGA digi data...");
    ROOT::RDF::Experimental::AddProgressBar(data);
    data.Foreach(
        [&](Long64_t time, unsigned channelID, float energy) {
            if (time > daqTimePicoseconds) { daqTimePicoseconds = time; }
            auto& ch{flatChannelSummary[channelID]};
            ch.channelID = channelID;
            ch.meanEnergy += energy;
            ++ch.triggerCount;
        },
        {"time", "channelID", "energy"});
    Mustard::PrintLn("Completed.");
    const auto daqTime{daqTimePicoseconds * CLHEP::ps};
    for (auto&& [_, ch] : flatChannelSummary) {
        ch.meanEnergy /= ch.triggerCount;
    }
    const auto channelSummaryText{FormatChannelSummary(cli->get<double>("--daq-t0") * CLHEP::s, daqTime, flatChannelSummary)};
    Mustard::Print<'W'>("{}", channelSummaryText);
    Mustard::MakeTextTMacro(channelSummaryText, cli->get("--output-digi-summary"))->Write();

    // define new columns

    auto extendedData{
        data.Define(
                "normalizedEnergy",
                [&](unsigned channelID, float energy) -> float {
                    return energy / flatChannelSummary.at(channelID).meanEnergy;
                },
                {"channelID", "energy"})
            .Redefine(
                "time",
                [timeOffset = cli->get<double>("--daq-t0") * CLHEP::s](Long64_t time) {
                    return time * CLHEP::ps + timeOffset;
                },
                {"time"})};

    // main reconstruction loop

    Mustard::Data::Output<Musae::Data::LGAHit> lgaHitOutput{cli->get("--output-hit-tree"), "Reconstructed LGA hit data"};
    Mustard::Data::Output<Musae::Data::CRMuEvent> crMuEventOutput{cli->get("--output-event-tree"), "Reconstructed cosmic-ray muon event"};
    LGADigi* headDigi{};
    LGADigiMap<std::unique_ptr<LGADigi>> coincidentDigi; // {moduleID, edge} -> [digi...]

    int eventID{};
    ROOT::RDF::Experimental::AddProgressBar(data);
    Mustard::PrintLn("Reconstructing events...");
    extendedData.Foreach(
        [&](double time, unsigned channelID, float energy, float normalizedEnergy) {
            // insert coincidence hit
            if (headDigi == nullptr or
                std::abs(time - Get<"time">(*headDigi)) < lga.CoincidenceTimeWindow()) {
                const auto ch{lga.TryChannelInfo(channelID)};
                if (ch == nullptr) { return; }
                // make and insert a digi
                auto digi{std::make_unique<LGADigi>(time, channelID, energy, normalizedEnergy)};
                if (headDigi == nullptr) {
                    headDigi = digi.get();
                }
                coincidentDigi[ch->moduleID][ch->edge].emplace_back(std::move(digi));
                return;
            }

            // process coincidence hit
            do {
                // reconstruct hits
                const auto [eventHit, eventDigi]{ReconstructAllHit(coincidentDigi, eventID, hitReconstructionMethod)};
                if (eventHit.empty()) { break; }
                lgaHitOutput.Fill(eventHit);
                // reconstruct cosmic-ray muon event
                std::unique_ptr<CRMuEvent> crMuEvent;
                if (reconstructCRMu) {
                    crMuEvent = ReconstructCRMu(eventHit, crMuReconstructionMethod);
                    if (crMuEvent) {
                        crMuEventOutput.Fill(*crMuEvent);
                    } else {
                        Mustard::PrintWarning(fmt::format("Failed to reconstruct event {}", eventID));
                    }
                }
                // plot event
                if (plotEventRange.has_value() and
                    plotEventRange->front() <= eventID and eventID < plotEventRange->back()) {
                    PlotEvent(coincidentDigi, eventHit, eventDigi, crMuEvent.get());
                }
                ++eventID;
            } while (false);

            // reset coincidence hit
            headDigi = nullptr;
            coincidentDigi.clear();
        },
        {"time", "channelID", "energy", "normalizedEnergy"});
    Mustard::PrintLn("Completed.");

    lgaHitOutput.Write();
    if (reconstructCRMu) {
        crMuEventOutput.Write();
    }

    return EXIT_SUCCESS;
}
