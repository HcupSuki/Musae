#include "Musae/SimFlux/Analysis.h++"
#include "Musae/SimFlux/SD/LGASD.h++"
#include "Musae/Simulation/Digitizer/LGAFastDigitizer.h++"

#include "Mustard/Utility/PrettyLog.h++"

namespace Musae::SimFlux::inline SD {

auto LGASD::EndOfEvent(G4HCofThisEvent* hc) -> void {
    Simulation::LGASD::EndOfEvent(hc);
    if (const auto digitizer{dynamic_cast<LGAFastDigitizer*>(fDigitizer.get())}) {
        Analysis::Instance().SubmitLGAHitData(*digitizer->DigiCollection().GetVector());
    } else {
        Mustard::PrintError("Failed to determine LGA digitizer type");
    }
}

} // namespace Musae::SimFlux::inline SD
