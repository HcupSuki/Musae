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
#pragma once

#include "Mustard/Detector/Definition/DefinitionBase.h++"


namespace Musae::Detector::Definition {

class Model final : public Mustard::Detector::Definition::DefinitionBase {
private:
    struct Edge {
        G4ThreeVector v1, v2;
        
        Edge(const G4ThreeVector& vertex1, const G4ThreeVector& vertex2) {
            // Store edge vertices in sorted order for comparison
            if (vertex1 < vertex2) {
                v1 = vertex1;
                v2 = vertex2;
            } else {
                v1 = vertex2;
                v2 = vertex1;
            }
        }
        
        bool operator<(const Edge& other) const {
            if (v1 != other.v1) return v1 < other.v1;
            return v2 < other.v2;
        }
    };

    auto ImportSTLModel(const G4String& filePath, const G4String& solidName);
    auto Construct(bool checkOverlaps) -> void override;

    
};

} // namespace Musae::Detector::Definition
