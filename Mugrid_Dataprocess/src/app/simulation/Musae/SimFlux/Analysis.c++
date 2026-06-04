#include "Musae/SimFlux/Analysis.h++"
#include "Musae/Simulation/Hit/LGAHit.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "fmt/core.h"

#include <optional>
#include <unordered_map>

namespace Musae::SimFlux {
    using namespace CLHEP;

Analysis::Analysis() :
    AnalysisBase{this}, fLga{Detector::Description::LGA::Instance()}, fRange{0}, fStep{0}, fHit{false} {}

auto Analysis::RunBeginUserAction(int runID) -> void {
    fLGASimHitOutput.emplace(fmt::format("G4Run{}/LGASimHit", runID));
    fCRMuSimEventOutput.emplace(fmt::format("G4Run{}/CRMuSimEvent", runID));
}

auto Analysis::EventEndUserAction() -> void {
    if (fLGAHitData && !fLGAHitData->empty()) {
        fLGASimHitOutput->Fill(*fLGAHitData);

        // Group digi by DetID
        std::unordered_map<int, std::vector<gsl::owner<LGAFastDigi*>>> hitsByDet;
        for (auto* digi : *fLGAHitData) {
            hitsByDet[Get<"DetID">(*digi)].push_back(digi);
        }

        for (auto& [detID, hits] : hitsByDet) {
            const auto cRMuEvent{fTruthFitter(hits)};
            if (cRMuEvent) {
                Get<"Range">(*cRMuEvent) = fRange / (g/cm2);
                fCRMuSimEventOutput->Fill(*cRMuEvent);
            }
        }
    }

    fLGAHitData = nullptr;
    fRange = 0;
    fHit = false;
}

auto Analysis::RunEndUserAction(int) -> void {
    // write data
    fLGASimHitOutput->Write();
    fCRMuSimEventOutput->Write();
    // reset output
    fLGASimHitOutput.reset();
    fCRMuSimEventOutput.reset();
}

} // namespace Musae::SimFlux
