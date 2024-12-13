#pragma once

#include "Musae/Data/Event.h++"
#include "Musae/Data/SimHit.h++"
#include "Musae/Reconstruction/Fitter/TruthFitter.h++"
#include "Musae/Simulation/Digi/LGAFastDigi.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Data/Tuple.h++"
#include "Mustard/Simulation/AnalysisBase.h++"

#include "gsl/gsl"

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
    auto SubmitLGAHitData(const std::vector<gsl::owner<LGAFastDigi*>>& dc) -> void { fLGAHitData = &dc; }

private:
    auto RunBeginUserAction(int runID) -> void override;
    auto EventEndUserAction() -> void override;
    auto RunEndUserAction(int) -> void override;

private:
    std::optional<Mustard::Data::Output<Data::LGASimHit>> fLGASimHitOutput;
    std::optional<Mustard::Data::Output<Data::CRMuSimEvent>> fCRMuSimEventOutput;

    const std::vector<gsl::owner<LGAFastDigi*>>* fLGAHitData;

    TruthFitter<> fTruthFitter;
};

} // namespace Musae::SimFlux
