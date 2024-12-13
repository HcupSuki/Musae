#include "Musae/SimFlux/Analysis.h++"
#include "Musae/Simulation/Hit/LGAHit.h++"

#include "fmt/core.h"

#include <optional>

namespace Musae::SimFlux {

auto Analysis::RunBeginUserAction(int runID) -> void {
    fLGASimHitOutput.emplace(fmt::format("G4Run{}/LGASimHit", runID));
    fCRMuSimEventOutput.emplace(fmt::format("G4Run{}/CRMuSimEvent", runID));
}

auto Analysis::EventEndUserAction() -> void {
    const auto crMuEvent{fLGAHitData ? std::optional{fTruthFitter(*fLGAHitData)} : std::nullopt};
    if (fLGAHitData) { fLGASimHitOutput->Fill(*fLGAHitData); }
    if (crMuEvent and *crMuEvent) { fCRMuSimEventOutput->Fill(**crMuEvent); }
    fLGAHitData = nullptr;
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
