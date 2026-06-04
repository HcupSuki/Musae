#include "Musae/Detector/Definition/Model.h++"
#include "Musae/Detector/Description/Model.h++"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4PVPlacement.hh"
#include "G4Transform3D.hh"
#include "G4TessellatedSolid.hh"
#include "G4TriangularFacet.hh"
#include "G4ThreeVector.hh"

#include <fstream>
#include <iostream>
#include <set>
#include <map>

namespace Musae::Detector::Definition {

namespace {
constexpr bool kModelPlacementDebug{false};
}

auto Model::ImportSTLModel(const G4String& filePath, const G4String& solidName) 
{
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error{"Error: Could not open STL file: " + filePath };
    }
    auto tessellated{std::make_unique<G4TessellatedSolid>(solidName)};
    
    // Data structures for model validation
    std::map<Edge, int> edgeCount;  // Edge occurrence count
    std::set<G4ThreeVector> vertices;  // All vertices
    int facetCount = 0;
    
    // Detect whether file is ASCII or binary STL
    char header[6];
    file.read(header, 5);
    header[5] = '\0';
    file.seekg(0);
    
    if (std::string(header) == "solid") {
        // ASCII STL format
        std::string line;
        G4ThreeVector vertex[3];
        G4ThreeVector normal;
        int vertexIndex = 0;
        
        while (std::getline(file, line)) {
            if (line.find("facet normal") != std::string::npos) {
                std::istringstream ss(line.substr(13));
                ss >> normal[0] >> normal[1] >> normal[2];
                vertexIndex = 0;
            }
            else if (line.find("vertex") != std::string::npos) {
                std::istringstream ss(line.substr(7));
                ss >> vertex[vertexIndex][0] >> vertex[vertexIndex][1] >> vertex[vertexIndex][2];
                vertices.insert(vertex[vertexIndex]);
                vertexIndex++;
                
                if (vertexIndex == 3) {
                    // Create a triangular facet
                    G4TriangularFacet* facet = new G4TriangularFacet(
                        vertex[0], vertex[1], vertex[2], ABSOLUTE);
                    tessellated->AddFacet(facet);
                    facetCount++;
                    
                    // Add three edges for statistics
                    Edge edges[3] = {
                        Edge(vertex[0], vertex[1]),
                        Edge(vertex[1], vertex[2]),
                        Edge(vertex[2], vertex[0])
                    };
                    
                    for (const auto& edge : edges) {
                        edgeCount[edge]++;
                    }
                }
            }
        }
    } else {
        // Binary STL format
        char header[80];
        file.read(header, 80);
        
        uint32_t numFacets;
        file.read(reinterpret_cast<char*>(&numFacets), 4);
        
        for (uint32_t i = 0; i < numFacets; ++i) {
            float normal[3];
            float vertices_data[9];
            uint16_t attrByteCount;
            
            file.read(reinterpret_cast<char*>(normal), 12);
            file.read(reinterpret_cast<char*>(vertices_data), 36);
            file.read(reinterpret_cast<char*>(&attrByteCount), 2);
            
            G4ThreeVector v[3];
            for (int j = 0; j < 3; ++j) {
                v[j] = G4ThreeVector(vertices_data[j*3], vertices_data[j*3+1], vertices_data[j*3+2]);
                vertices.insert(v[j]);
            }
            
            G4TriangularFacet* facet = new G4TriangularFacet(
                v[0], v[1], v[2], ABSOLUTE);
            tessellated->AddFacet(facet);
            facetCount++;
            
            // Add three edges for statistics
            Edge edges[3] = {
                Edge(v[0], v[1]),
                Edge(v[1], v[2]),
                Edge(v[2], v[0])
            };
            
            for (const auto& edge : edges) {
                edgeCount[edge]++;
            }
        }
    }
    
    file.close();
    tessellated->SetSolidClosed(true);
    
    // Validate model
    bool isClosed = true;
    int nonManifoldEdges = 0;
    
    for (const auto& entry : edgeCount) {
        if (entry.second != 2) {
            isClosed = false;
            if (entry.second > 2) nonManifoldEdges++;
        }
    }
    
    if (nonManifoldEdges > 0) {
        throw std::runtime_error{
            "Error: Model is not manifold, contains " + std::to_string(nonManifoldEdges) + " non-manifold edges. Model path: " + filePath};
    }
    if (!isClosed)
    {
        throw std::runtime_error{
            "Error: Model is not closed. Model path: " + filePath};
    }
    
    return tessellated;
}

auto Model::Construct(bool checkOverlaps) -> void {
    const auto& Model{Description::Model::Instance()};
    // std::cout << "num of models: " << ssize(Model.Path()) << std::endl;

    if (kModelPlacementDebug && ssize(Model.Path()) > 1) {
        G4cout << "[ModelDebug] Multiple models are created as siblings under the same mother volume."
               << " If one is inside another, this is an overlap configuration and navigation can become ambiguous."
               << G4endl;
    }

    for(gsl::index i{}; i < ssize(Model.Path()); ++i) {
        // std::cout << "Model Material: " << Model.Material().at(i).c_str() << std::endl;
        auto tessellatedSolid{ImportSTLModel(Model.Path().at(i), "Model_" + std::to_string(i))};
        G4Material* material = G4NistManager::Instance()->FindOrBuildMaterial(Model.Material().at(i).c_str());
        if (!material) {
            material = G4Material::GetMaterial(Model.Material().at(i).c_str());
            if (!material) {
                throw std::runtime_error{"ERROR: Material '"};
            }
        }
        // G4double density = material->GetDensity();
    
        // // GetDensity() returns density in g/cm^3
        // // Use G4UnitDefinition to convert density to different units
        // G4cout << "Material: " << material->GetName() << G4endl;
        // G4cout << "Density: " << density << "" << G4endl;

        const auto logic{Make<G4LogicalVolume>(
            tessellatedSolid.release(),
            material,
            "Model_" + std::to_string(i))};
        const auto transform{Model.Transform()};
        const auto placement{Make<G4PVPlacement>(
            Model.Transform(),
            logic,
            "Model_" + std::to_string(i),
            Mother().LogicalVolume(),
            false,
            0,
            checkOverlaps)};

        if (kModelPlacementDebug) {
            const auto translation{transform.getTranslation()};
            G4cout << "[ModelDebug] Place " << placement->GetName()
                   << " path=" << Model.Path().at(i)
                   << " material=" << material->GetName()
                   << " at=" << translation
                   << " checkOverlaps=" << checkOverlaps
                   << G4endl;

            if (checkOverlaps) {
                const auto hasOverlap{placement->CheckOverlaps(200000, 0, true)};
                G4cout << "[ModelDebug]   overlapResult(" << placement->GetName() << ") = "
                       << hasOverlap << G4endl;
            }
        }
    }

    
}

} // namespace Musae::Detector::Definition
