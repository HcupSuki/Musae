#pragma once

#include "Musae/Simulation/SD/LGASD.h++"

namespace Musae::SimFlux::inline SD {

class LGASD final : public Simulation::LGASD {
public:
    auto EndOfEvent(G4HCofThisEvent* hc) -> void override;
};

} // namespace Musae::SimFlux::inline SD
