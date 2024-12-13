#pragma once

#include "Musae/Data/SimHit.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Extension/Geant4X/Memory/UseG4Allocator.h++"

#include "G4TDigiCollection.hh"
#include "G4VDigi.hh"

namespace Musae::inline Simulation::inline Digi {

class LGAFastDigi final : public Mustard::Geant4X::UseG4Allocator<LGAFastDigi>,
                          public G4VDigi,
                          public Mustard::Data::Tuple<Data::LGASimHit> {
public:
    LGAFastDigi() = default;
    ~LGAFastDigi() = default;
    inline LGAFastDigi(const Tuple& t);
    inline LGAFastDigi(Tuple&& t);
};

using LGAFastDigiCollection = G4TDigiCollection<LGAFastDigi>;

} // namespace Musae::inline Simulation::inline Digi

#include "Musae/Simulation/Digi/LGAFastDigi.inl"
