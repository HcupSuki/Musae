#include "Musae/Simulation/Digitizer/LGADigitizerBase.h++"

#include <utility>

namespace Musae::inline Simulation::inline Digitizer {

LGADigitizerBase::LGADigitizerBase(std::string name) :
    NonMoveableBase{},
    G4VDigitizerModule{std::move(name)},
    fHitMap{} {}

} // namespace Musae::inline Simulation::inline Digitizer
