#include "Musae/Data/Digi.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/Detector/Description/LGA.h++"
#include "Musae/ReconHit/CLI.h++"
#include "Musae/ReconHit/Reconstruct.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Data/Tuple.h++"
#include "Mustard/Env/BasicEnv.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDataFrame.hxx"
#include "TFile.h"

#include "muc/ceta_string"
#include "muc/math"

#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <map>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

auto ProcessEventDigi(const std::map<std::pair<int, char>, std::vector<Mustard::Data::Tuple<Musae::Data::LGADigi>>>& eventDigi,
                      int eventID, std::string_view method, Mustard::Data::Output<Musae::Data::LGAHit>& output) -> bool;

auto main(int argc, char* argv[]) -> int {
    Musae::ReconHit::CLI cli;
    Mustard::Env::BasicEnv env{argc, argv, cli};

    ROOT::RDataFrame data{cli.InputTreeName(), cli.InputFilePath()};
    ROOT::RDF::Experimental::AddProgressBar(data);

    TFile file{cli.OutputFilePath().c_str(), cli.OutputFileMode().c_str()};
    Mustard::Data::Output<Musae::Data::LGAHit> output{"LGAHit", "Reconstructed LGA hit data"};

    const auto& lga{Musae::Detector::Description::LGA::Instance()};
    const auto coincidenceTimeWindow{std::llround(lga.CoincidenceTimeWindow() / CLHEP::ps)};

    const Mustard::Data::Tuple<Musae::Data::LGADigi>* headDigi{};
    std::map<std::pair<int, char>, std::vector<Mustard::Data::Tuple<Musae::Data::LGADigi>>> eventDigi; // {moduleID, edge} -> [digi...]
    int eventID{};
    data.Foreach(
        [&](Long64_t time, unsigned channelID, float energy) {
            // insert coincidence hit
            if (headDigi == nullptr or
                std::abs(time - Get<"time">(*headDigi)) < coincidenceTimeWindow) {
                std::pair<int, char> moduleIDAndEdge;
                try {
                    const auto& ch{lga.ChannelInfo(channelID)};
                    moduleIDAndEdge = {ch.moduleID, ch.edge};
                } catch (const std::out_of_range&) {
                    return;
                }
                const auto& digi{eventDigi[moduleIDAndEdge].emplace_back(time, channelID, energy)};
                if (headDigi == nullptr) {
                    headDigi = &digi;
                }
                return;
            }
            // process coincidence hit
            if (ProcessEventDigi(eventDigi, eventID, "Weighted2D", output)) {
                ++eventID;
            }
            // reset coincidence hit
            headDigi = nullptr;
            eventDigi.clear();
        },
        {"time", "channelID", "energy"});

    output.Write();

    return EXIT_SUCCESS;
}

auto ProcessEventDigi(const std::map<std::pair<int, char>, std::vector<Mustard::Data::Tuple<Musae::Data::LGADigi>>>& eventDigi,
                      int eventID, std::string_view method, Mustard::Data::Output<Musae::Data::LGAHit>& output) -> bool {
    const auto& lga{Musae::Detector::Description::LGA::Instance()};
    const auto nModule{lga.NModule()};
    const auto minLuminous{lga.NLuminousFiberThresholdPerDirection()};

    std::vector<std::unique_ptr<Mustard::Data::Tuple<Musae::Data::LGAHit>>> eventHit(nModule);
    int hitID{};
    for (auto moduleID : std::views::iota(0, nModule)) {
        std::unordered_map<char, std::vector<const Mustard::Data::Tuple<Musae::Data::LGADigi>*>> digiOfThisModule;
        for (auto edge : {'x', 'y'}) {
            // event selection
            const auto iDigiOfTheEdge{eventDigi.find({moduleID, edge})};
            if (iDigiOfTheEdge == eventDigi.cend()) {
                return false;
            }
            const auto& digiOfTheEdge{iDigiOfTheEdge->second};
            if (ssize(digiOfTheEdge) < minLuminous) {
                return false;
            }
            // insert digi
            for (auto&& digi : digiOfTheEdge) {
                digiOfThisModule[edge].emplace_back(&digi);
            }
        }
        // reconstruct hit
        auto hit{Musae::ReconHit::Reconstruct(digiOfThisModule, eventID, hitID, method)};
        if (hit) {
            eventHit.at(moduleID) = std::move(hit);
            ++hitID;
        }
    }

    if (std::ranges::any_of(eventHit, [](auto&& hit) { return hit == nullptr; })) {
        return false;
    }

    output.Fill(eventHit);
    return true;
}
