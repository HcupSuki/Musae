#pragma once

#include "Musae/Simulation/Digi/LGAFastDigi.h++"
#include "Musae/Simulation/Hit/LGAHit.h++"

#include "Mustard/Utility/NonMoveableBase.h++"

#include "G4VDigitizerModule.hh"

#include "muc/hash_map"

#include <string>
#include <vector>

namespace Musae::inline Simulation::inline Digitizer {

class LGADigitizerBase : public Mustard::NonMoveableBase,
                         public G4VDigitizerModule {
public:
    LGADigitizerBase(std::string name);
    virtual ~LGADigitizerBase() = default;

    auto HitMap(const auto& hc) -> void { fHitMap = &hc; }

protected:
    const muc::flat_hash_map<int, std::vector<LGAHit*>>* fHitMap;
};

} // namespace Musae::inline Simulation::inline Digitizer
