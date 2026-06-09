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
