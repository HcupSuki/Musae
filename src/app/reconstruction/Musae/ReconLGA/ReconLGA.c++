#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconLGA/FormatChannelSummary.h++"
#include "Musae/ReconLGA/PlotEvent.h++"
#include "Musae/ReconLGA/ReconLGA.h++"
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
#include "TNamed.h"

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

using namespace std::string_literals;

auto ReconLGA::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("input").help("Input file path(s).").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--input-tree").help("Input tree name.").required().nargs(1).default_value("data"s);
    cli->add_argument("-n", "--input-range").help("Input entry range.").nargs(2).scan<'i', unsigned>();
    cli->add_argument("-o", "--output").help("Output file path.").required().nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").required().nargs(1).default_value("NEW"s);
    cli->add_argument("-s", "--output-digi-summary").help("Output digi summary name.").required().nargs(1).default_value("LGADigiSummary"s);
    cli->add_argument("-h", "--output-digi-tree").help("Output digi tree name.").required().nargs(1).default_value("LGADigi"s);
    cli->add_argument("-h", "--output-hit-tree").help("Output hit tree name.").required().nargs(1).default_value("LGAHit"s);
    cli->add_argument("-e", "--output-event-tree").help("Output event tree name.").required().nargs(1).default_value("CRMuEvent"s);
    cli->add_argument("-p", "--plot-hit").help("Produce hit plots for an event range (e.g. -p <first> <last>).").nargs(2).scan<'i', int>();
    cli->add_argument("-d", "--soft-dead-time").help("Artificial dead time (in millisecond).").required().nargs(1).scan<'g', double>().default_value(0.);
    cli->add_argument("-r", "--hit-method").help("Hit reconstruction method.").required().nargs(1).default_value("EnergyWeighted2D"s);
    cli->add_argument("-c", "--no-crmu").help("Skip cosmic-ray muon event reconstruction.").flag();
    cli->add_argument("-r", "--crmu-method").help("Cosmic-ray muon event reconstruction method.").required().nargs(1).default_value("LeastChiSquare"s);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    const auto plotEventRange{cli->present<std::vector<int>>("--plot-hit")};
    const auto softDeadTime{cli->get<double>("--soft-dead-time") * CLHEP::ms};
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

    muc::flat_hash_map<int, ChannelSummary> flatChannelSummary;
    Mustard::PrintLn("Summarizing LGA digi data...");
    ROOT::RDF::Experimental::AddProgressBar(data);
    data.Foreach(
        [&](unsigned channelID, float energy) {
            auto& ch{flatChannelSummary[channelID]};
            ch.channelID = channelID;
            ch.meanEnergy += energy;
            ++ch.triggerCount;
        },
        {"channelID", "energy"});
    Mustard::PrintLn("Completed.");
    for (auto&& [_, ch] : flatChannelSummary) {
        ch.meanEnergy /= ch.triggerCount;
    }
    const auto channelSummaryText{FormatChannelSummary(flatChannelSummary)};
    Mustard::Print<'W'>("{}", channelSummaryText);
    Mustard::MakeTextTMacro(channelSummaryText, cli->get("--output-digi-summary"))->Write();

    // define new columns

    auto extendedData{
        data.Define(
                "NormalizedEnergy",
                [&](unsigned channelID, float energy) -> float {
                    return energy / flatChannelSummary.at(channelID).meanEnergy;
                },
                {"channelID", "energy"})
            .Redefine(
                "time",
                [](Long64_t time) {
                    return time * CLHEP::ps;
                },
                {"time"})};

    // main reconstruction loop

    Mustard::Data::Output<Musae::Data::LGADigi> lgaDigiOutput{cli->get("--output-digi-tree"), "Reconstructed LGA hit digi"};
    Mustard::Data::Output<Musae::Data::LGAHit> lgaHitOutput{cli->get("--output-hit-tree"), "Reconstructed LGA hit data"};
    Mustard::Data::Output<Musae::Data::CRMuEvent> crMuEventOutput{cli->get("--output-event-tree"), "Reconstructed cosmic-ray muon event"};
    std::optional<double> triggerTime{};
    std::optional<double> eventCloseTime{};
    LGADigiMap<std::unique_ptr<LGADigi>> coincidentDigi; // {moduleID, edge} -> [digi...]

    int eventID{};
    ROOT::RDF::Experimental::AddProgressBar(data);
    Mustard::PrintLn("Reconstructing events...");
    extendedData.Foreach(
        [&](double time, unsigned channelID, float energy, float normalizedEnergy) {
            // insert coincidence hit
            if (eventCloseTime and std::abs(time - *eventCloseTime) < softDeadTime) {
                return;
            }
            if (triggerTime == std::nullopt) {
                triggerTime = time;
            }
            if (std::abs(time - *triggerTime) < lga.CoincidenceTimeWindow()) {
                const auto ch{lga.TryChannelInfo(channelID)};
                if (ch == nullptr) { return; }
                // make and insert a digi
                auto digi{std::make_unique<LGADigi>(time, channelID, energy, eventID,
                                                    ch->moduleID, ch->edge, ch->fiberLocalID,
                                                    normalizedEnergy)};
                coincidentDigi[ch->moduleID][ch->edge].emplace_back(std::move(digi));
                return;
            }

            // process coincidence hit
            [&] {
                // reconstruct hits
                const auto [eventDigi, eventHit]{ReconstructAllHit(coincidentDigi, hitReconstructionMethod)};
                if (eventHit.empty()) { return; }
                for (auto&& [_, moduleDigi] : std::as_const(eventDigi)) {
                    for (auto&& [_, digi] : std::as_const(moduleDigi)) {
                        lgaDigiOutput.Fill(digi);
                    }
                }
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
                    PlotEvent(coincidentDigi, eventDigi, eventHit, crMuEvent.get());
                }
                eventCloseTime = time;
                ++eventID;
            }();

            // reset coincidence hit
            triggerTime = std::nullopt;
            coincidentDigi.clear();
        },
        {"time", "channelID", "energy", "NormalizedEnergy"});
    Mustard::PrintLn("Completed.");

    const auto totalSoftDeadTime{eventID * softDeadTime};
    TNamed{fmt::format("Soft dead time: {:.1f} s", totalSoftDeadTime / CLHEP::s),
           fmt::format("Total soft dead time: {} s, accumulated from {} events' {}-ms dead time",
                       totalSoftDeadTime / CLHEP::s, eventID, softDeadTime / CLHEP::ms)}
        .Write();

    lgaDigiOutput.Write();
    lgaHitOutput.Write();
    if (reconstructCRMu) {
        crMuEventOutput.Write();
    }

    return EXIT_SUCCESS;
}

} // namespace Musae::ReconLGA
