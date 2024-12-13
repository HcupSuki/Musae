#pragma once

#include "Musae/Simulation/Digi/LGAFastDigi.h++"
#include "Musae/Simulation/Hit/LGAHit.h++"

#include "Mustard/Utility/NonMoveableBase.h++"

#include "G4VDigitizerModule.hh"

#include <string>
#include <unordered_map>
#include <vector>

namespace Musae::inline Simulation::inline Digitizer {

class LGADigitizerBase : public Mustard::NonMoveableBase,
                         public G4VDigitizerModule {
public:
    LGADigitizerBase(std::string name);
    virtual ~LGADigitizerBase() = default;

    auto HitMap(const auto& hc) -> void { fHitMap = &hc; }

protected:
    const std::unordered_map<int, std::vector<LGAHit*>>* fHitMap;
};

} // namespace Musae::inline Simulation::inline Digitizer
