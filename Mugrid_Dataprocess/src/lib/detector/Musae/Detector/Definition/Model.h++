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
