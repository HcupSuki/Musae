#include "Musae/Detector/Definition/LGA.h++"
#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Utility/PrettyLog.h++"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Transform3D.hh"

#include "fmt/core.h"

namespace Musae::Detector::Definition {

auto LGA::Construct(bool checkOverlaps) -> void {
    const auto& lga{Description::LGA::Instance()};
    { // Scintillator
        const auto name{fmt::format("{}Scintillator", lga.Name())};
        const auto solid{Make<G4Box>(
            name,
            lga.ScintillatorWidth() / 2,
            lga.ScintillatorWidth() / 2,
            lga.ScintillatorThickness() / 2)};
        const auto logic{Make<G4LogicalVolume>(
            solid,
            G4NistManager::Instance()->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE"),
            name)};
        for (int i{}; i < lga.NModule(); ++i) {
            Make<G4PVPlacement>(
                lga.Transform(i, -lga.ScintillatorThickness() / 2),
                logic,
                name,
                Mother().LogicalVolume(),
                false,
                i,
                checkOverlaps);
        }
    }
    if (lga.UseFastLGA()) {
        const auto name{lga.Name()};
        const auto width{lga.LGACellWidth() * lga.NLGACellXY()};
        const auto solid{Make<G4Box>(
            name,
            width / 2,
            width / 2,
            lga.LGAThickness() / 2)};
        const auto logic{Make<G4LogicalVolume>(
            solid,
            G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"),
            name)};
        for (int i{}; i < lga.NModule(); ++i) {
            Make<G4PVPlacement>(
                lga.Transform(i, lga.LGAThickness() / 2),
                logic,
                name,
                Mother().LogicalVolume(),
                false,
                i,
                checkOverlaps);
        }
    } else {
        Mustard::PrintError("Full LGA has not been implemented yet");
    }
}

} // namespace Musae::Detector::Definition
