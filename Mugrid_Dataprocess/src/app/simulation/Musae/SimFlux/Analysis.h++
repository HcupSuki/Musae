#pragma once

#include "Musae/Data/Event.h++"
#include "Musae/Data/SimHit.h++"
#include "Musae/Reconstruction/Fitter/TruthFitter.h++"
#include "Musae/Simulation/Digi/LGAFastDigi.h++"
#include "Musae/Detector/Description/LGA.h++"


#include "Mustard/Data/Output.h++"
#include "Mustard/Data/Tuple.h++"
#include "Mustard/Simulation/AnalysisBase.h++"

#include "gsl/gsl"

#include "globals.hh"

#include <filesystem>
#include <memory>
#include <utility>

class TFile;

namespace Musae::inline Simulation::inline Hit {
class LGAHit;
} // namespace Musae::inline Simulation::inline Hit

namespace Musae::SimFlux {

class Analysis final : public Mustard::Simulation::AnalysisBase<Analysis, "SimFlux"> {
public:
    Analysis();

    auto SubmitLGAHitData(const std::vector<gsl::owner<LGAFastDigi*>>& dc) -> void { fLGAHitData = &dc; }
    auto AddRange(G4double r) -> void { fRange += r; }
    auto AddStep(G4double r) -> void { fStep += r; }
    auto StepSum() -> G4double { return fStep; }
    auto GetHit() -> bool { return fHit; }
    auto SetHit(bool hit) -> void { fHit = hit; }

private:
    auto RunBeginUserAction(int runID) -> void override;
    auto EventEndUserAction() -> void override;
    auto RunEndUserAction(int) -> void override;

private:
    const Musae::Detector::Description::LGA & fLga;
    std::optional<Mustard::Data::Output<Data::LGASimHit>> fLGASimHitOutput;
    std::optional<Mustard::Data::Output<Data::CRMuSimEvent>> fCRMuSimEventOutput;
    G4double fRange;
    G4double fStep;
    bool fHit;

    const std::vector<gsl::owner<LGAFastDigi*>>* fLGAHitData;

    TruthFitter<> fTruthFitter;
};

} // namespace Musae::SimFlux
