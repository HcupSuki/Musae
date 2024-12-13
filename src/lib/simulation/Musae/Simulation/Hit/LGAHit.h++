#pragma once

#include "Musae/Data/SimHit.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Extension/Geant4X/Memory/UseG4Allocator.h++"

#include "G4THitsCollection.hh"
#include "G4VHit.hh"

namespace Musae::inline Simulation::inline Hit {

class LGAHit final : public Mustard::Geant4X::UseG4Allocator<LGAHit>,
                     public G4VHit,
                     public Mustard::Data::Tuple<Data::LGASimHit> {
public:
    LGAHit() = default;
    inline LGAHit(const Tuple& t);
    inline LGAHit(Tuple&& t);
};

using LGAHitsCollection = G4THitsCollection<LGAHit>;

} // namespace Musae::inline Simulation::inline Hit

#include "Musae/Simulation/Hit/LGAHit.inl"
