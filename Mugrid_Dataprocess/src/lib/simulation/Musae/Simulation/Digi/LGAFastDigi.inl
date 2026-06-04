namespace Musae::inline Simulation::inline Digi {

inline LGAFastDigi::LGAFastDigi(const Tuple& t) :
    UseG4Allocator{},
    G4VDigi{},
    Tuple{t} {}

inline LGAFastDigi::LGAFastDigi(Tuple&& t) :
    UseG4Allocator{},
    G4VDigi{},
    Tuple{std::move(t)} {}

} // namespace Musae::inline Simulation::inline Digi
