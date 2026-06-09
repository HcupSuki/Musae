// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
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
