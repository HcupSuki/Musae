#include "Musae/Data/Digi.h++"
#include "Musae/Data/Event.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/VisLGA/EventIDFilter.h++"
#include "Musae/VisLGA/EventPlot.h++"
#include "Musae/VisLGA/Type.h++"
#include "Musae/VisLGA/VisLGA.h++"

#include "Mustard/Data/SeqProcessor.h++"
#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/Print.h++"

#include "ROOT/RDataFrame.hxx"
#include "TApplication.h"
#include "TFile.h"

#include "muc/ptrvec"

#include "fmt/core.h"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace Musae::VisLGA {

VisLGA::VisLGA() :
    Subprogram{"VisLGA", "LGA event display."} {}

using namespace std::string_literals;

auto VisLGA::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("input").help("Input file path(s).").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-n", "--display-event").help("Event ID ranges for events to be displayed. Format: <first>..<last> ... (e.g. 2..3 4..100)").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-g", "--input-digi-tree").help("Input digi tree name.").default_value("LGADigi"s).required().nargs(1);
    cli->add_argument("-h", "--input-hit-tree").help("Input hit tree name.").default_value("LGAHit"s).required().nargs(1);
    cli->add_argument("-e", "--input-event-tree").help("Input event tree name.").default_value("CRMuEvent"s).required().nargs(1);
    cli->add_argument("-o", "--output").help("Output file path.");
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").default_value("NEW"s).required().nargs(1);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    ROOT::RDataFrame fullLGADigiData{cli->get("--input-digi-tree"), cli->get<std::vector<std::string>>("input")};
    ROOT::RDataFrame fullLGAHitData{cli->get("--input-hit-tree"), cli->get<std::vector<std::string>>("input")};
    ROOT::RDataFrame fullCRMuEventData{cli->get("--input-event-tree"), cli->get<std::vector<std::string>>("input")};

    std::unique_ptr<TFile> outputFile;
    if (const auto outputFilePath{cli->present("--output")}) {
        outputFile = std::make_unique<TFile>(outputFilePath->c_str(), cli->get("--output-mode").c_str());
        if (not outputFile->IsOpen()) {
            return EXIT_FAILURE;
        }
    }

    std::unique_ptr<TApplication> rootApp;
    if (outputFile == nullptr) {
        rootApp = std::make_unique<TApplication>(Name().c_str(), nullptr, nullptr);
    }

    const EventIDFilter Filter{cli->get<std::vector<std::string>>("--display-event")};
    auto lgaDigiData{fullLGADigiData.Filter(Filter, {"EvtID"})};
    auto lgaHitData{fullLGAHitData.Filter(Filter, {"EvtID"})};
    auto cRMuEventData{fullCRMuEventData.Filter(Filter, {"EvtID"})};

    Mustard::Data::SeqProcessor processor;
    processor.PrintProgress(false);
    processor.Process<Musae::Data::LGADigi, Musae::Data::LGAHit, Musae::Data::CRMuEvent>(
        {lgaDigiData, lgaHitData, cRMuEventData}, int{}, "EvtID",
        [&](const muc::shared_ptrvec<LGADigi>& lgaDigi,
            const muc::shared_ptrvec<LGAHit>& lgaHit,
            const muc::shared_ptrvec<CRMuEvent>& cRMuEvent) {
            if (lgaDigi.empty()) {
                Mustard::PrintError("Empty LGA digi data, skipping the event");
                return;
            }
            const auto eventID{*Get<"EvtID">(*lgaDigi.front())};
            if (cRMuEvent.size() > 1) {
                Mustard::PrintWarning(fmt::format("Event {} has more than one cosmic-ray muon event", eventID));
            }
            const CRMuEvent* theCRMuEvent{};
            if (not cRMuEvent.empty()) {
                theCRMuEvent = cRMuEvent.front().get();
            }

            auto canvas{EventPlot(lgaDigi, lgaHit, theCRMuEvent)};
            if (outputFile) {
                canvas->Write();
            } else {
                canvas->Update();
                Mustard::Print("Press enter to display next event...");
                std::getchar();
            }
        });

    if (outputFile == nullptr) {
        Mustard::Print("Press enter to exit...");
        std::getchar();
    }

    return EXIT_SUCCESS;
}

} // namespace Musae::VisLGA
