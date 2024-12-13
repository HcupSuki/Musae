#pragma once

#include "Musae/Detector/Description/LGA.h++"
#include "Musae/Detector/Description/Terrain.h++"
#include "Musae/Detector/Description/World.h++"

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "G4VUserDetectorConstruction.hh"

#include <memory>

namespace Mustard::Detector::Definition {
class DefinitionBase;
} // namespace Mustard::Detector::Definition

namespace Musae::SimFlux {

// inline namespace SD {
// class EarthSD;
// } // namespace SD

inline namespace Action {

class DetectorConstruction final : public Mustard::Env::Memory::PassiveSingleton<DetectorConstruction>,
                                   public G4VUserDetectorConstruction {
public:
    DetectorConstruction(bool checkOverlap);

    auto Construct() -> G4VPhysicalVolume* override;

public:
    using DescriptionInUse = std::tuple<Detector::Description::LGA,
                                        Detector::Description::Terrain,
                                        Detector::Description::World>;

private:
    bool fCheckOverlap;

    std::unique_ptr<Mustard::Detector::Definition::DefinitionBase> fWorld;
};

} // namespace Action

} // namespace Musae::SimFlux
