#include "Musae/Detector/Definition/LGA.h++"
#include "Musae/Detector/Definition/Terrain.h++"
#include "Musae/Detector/Definition/World.h++"
#include "Musae/SimFlux/Action/DetectorConstruction.h++"
#include "Musae/SimFlux/Messenger/DetectorMessenger.h++"
#include "Musae/SimFlux/SD/LGASD.h++"

namespace Musae::SimFlux::inline Action {

DetectorConstruction::DetectorConstruction(bool checkOverlap) :
    PassiveSingleton{this},
    G4VUserDetectorConstruction{},
    fCheckOverlap{checkOverlap} {
    DetectorMessenger::EnsureInstantiation();
}

auto DetectorConstruction::Construct() -> G4VPhysicalVolume* {
    fWorld = std::make_unique<Detector::Definition::World>();
    auto& terrain{fWorld->NewDaughter<Detector::Definition::Terrain>(fCheckOverlap)};
    auto& lga{terrain.NewDaughter<Detector::Definition::LGA>(fCheckOverlap)};

    lga.RegisterSD("LGAScintillator", new LGASD);

    return fWorld->PhysicalVolume();
}

} // namespace Musae::SimFlux::inline Action
