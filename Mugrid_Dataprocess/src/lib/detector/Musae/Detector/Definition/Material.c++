#include "Musae/Detector/Definition/Material.h++"
#include "Musae/Detector/Description/Material.h++"
#include "Mustard/Utility/NonMoveableBase.h++"

#include "G4NistManager.hh"
#include "G4GDMLParser.hh"
#include "G4Material.hh"
#include "G4MaterialTable.hh"


#include <iostream>
#include <fstream>
#include <exception>


namespace Musae::Detector::Definition {

auto Material::LoadMaterial() -> void {
    const auto& Material{Description::Material::Instance()};
    if (Material.Path().empty()) {
        std::cerr << "Warning: Material description is empty." << std::endl;
        return;
    }
    const auto GdmlPath{Material.Path()};

    std::ifstream fileCheck(GdmlPath);
    if (!fileCheck) {
        throw std::runtime_error("GDML file not found: " + GdmlPath);
    }
    fileCheck.close();

    G4GDMLParser parser;

    try {
        // Read GDML file - parse materials only
        parser.Read(GdmlPath, false);

        // Check if materials loaded successfully
        const G4GDMLAuxMapType* auxmap = parser.GetAuxMap();
        if (!auxmap) {
            std::cout << "GDML auxiliary information is not available." << std::endl;
        }

        // Verify materials loaded successfully
        // G4MaterialTable* matTable = G4Material::GetMaterialTable();
        // size_t loadedMaterialCount = matTable->size();

        // // Print all loaded materials (optional)
        // std::cout << "Loaded materials: " << std::endl;
        // for (size_t i = 0; i < loadedMaterialCount; i++) {
        //     G4Material* mat = (*matTable)[i];
        //     std::cout << " - " << mat->GetName() << std::endl;
        // }
    } catch (const std::exception& e) {
        std::cerr << "Error while parsing GDML file: " << e.what() << std::endl;
    }

}

} // namespace Musae::Detector::Definition
