#include "Musae/Detector/Definition/LGA.h++"
#include "Musae/Detector/Description/LGA.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Transform3D.hh"
#include "G4ios.hh"

#include "fmt/core.h"

namespace Musae::Detector::Definition {

using namespace Mustard::LiteralUnit::Length;

namespace {
constexpr bool kLGAPlacementDebug{false};
}

auto LGA::Construct(bool checkOverlaps) -> void {
    const auto& lga{Description::LGA::Instance()};
    if (kLGAPlacementDebug) {
        G4cout << "[LGADefinition] Mother logical volume for LGA placement: "
               << Mother().LogicalVolume()->GetName()
               << ", checkOverlaps=" << checkOverlaps
               << ", nDetector=" << lga.NDetector()
               << G4endl;
    }

    const auto galactic{G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic")};

    // Envelope box dimensions
    const auto halfX{std::max(lga.ScintillatorWidthX(), lga.LGAWidthX()) / 2 + 1_cm};
    const auto halfY{std::max(lga.ScintillatorWidthY(), lga.LGAWidthY()) / 2 + 1_cm};
    const auto halfZ{((lga.NModule() - 1) * lga.ModuleSpacing() + lga.ScintillatorThickness() + lga.LGAThickness()) / 2 + 1_cm};

    // Scintillator (shared solid and logic)
    const auto scintName{fmt::format("{}Scintillator", lga.Name())};
    const auto scintSolid{Make<G4Box>(
        scintName,
        lga.ScintillatorWidthX() / 2,
        lga.ScintillatorWidthY() / 2,
        lga.ScintillatorThickness() / 2)};
    const auto scintLogic{Make<G4LogicalVolume>(
        scintSolid,
        G4NistManager::Instance()->FindOrBuildMaterial("G4_PLASTIC_SC_VINYLTOLUENE"),
        scintName)};

    // LGA (shared solid and logic)
    std::optional<G4LogicalVolume*> lgaLogic;
    if (lga.UseFastLGA()) {
        const auto lgaName{lga.Name()};
        const auto widthX{lga.LGACellWidth() * lga.NFiberX()};
        const auto widthY{lga.LGACellWidth() * lga.NFiberY()};
        const auto lgaSolid{Make<G4Box>(
            lgaName,
            widthX / 2,
            widthY / 2,
            lga.LGAThickness() / 2)};
        lgaLogic = Make<G4LogicalVolume>(
            lgaSolid,
            G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR"),
            lgaName);
    } else {
        Mustard::PrintError("Full LGA has not been implemented yet");
    }

    for (int detID{}; detID < lga.NDetector(); ++detID) {
        // Create envelope box for this detector
        const auto envName{fmt::format("{}Envelope_{}", lga.Name(), detID)};
        const auto envSolid{Make<G4Box>(envName, halfX, halfY, halfZ)};
        const auto envLogic{Make<G4LogicalVolume>(envSolid, galactic, envName)};

        Make<G4PVPlacement>(
            lga.EnvelopeTransform(detID),
            envLogic,
            envName,
            Mother().LogicalVolume(),
            false,
            detID,
            checkOverlaps);

        if (kLGAPlacementDebug) {
            G4cout << "[LGADefinition] Placed envelope " << envName
                   << " in " << Mother().LogicalVolume()->GetName()
                   << " copyNo=" << detID
                   << G4endl;
        }

        // Place scintillator modules inside envelope
        for (int i{}; i < lga.NModule(); ++i) {
            Make<G4PVPlacement>(
                lga.ModuleLocalTransform(i, -lga.ScintillatorThickness() / 2),
                scintLogic,
                scintName,
                envLogic,
                false,
                i,
                checkOverlaps);
        }

        // Place LGA modules inside envelope
        if (lgaLogic) {
            for (int i{}; i < lga.NModule(); ++i) {
                Make<G4PVPlacement>(
                    lga.ModuleLocalTransform(i, lga.LGAThickness() / 2),
                    *lgaLogic,
                    lga.Name(),
                    envLogic,
                    false,
                    i,
                    checkOverlaps);
            }
        }
    }
}

} // namespace Musae::Detector::Definition
