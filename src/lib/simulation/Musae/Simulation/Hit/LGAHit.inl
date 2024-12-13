namespace Musae::inline Simulation::inline Hit {

inline LGAHit::LGAHit(const Tuple& t) :
    UseG4Allocator{},
    G4VHit{},
    Tuple{t} {}

inline LGAHit::LGAHit(Tuple&& t) :
    UseG4Allocator{},
    G4VHit{},
    Tuple{std::move(t)} {}

} // namespace Musae::inline Simulation::inline Hit
