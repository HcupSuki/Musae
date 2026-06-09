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
#include "Musae/Detector/Definition/LGA.h++"
#include "Musae/Detector/Definition/Material.h++"
#include "Musae/Detector/Definition/Model.h++"
#include "Musae/Detector/Definition/Terrain.h++"
#include "Musae/Detector/Definition/World.h++"
#include "Musae/Detector/Description/Hierarchy.h++"
#include "Musae/SimFlux/Action/DetectorConstruction.h++"
#include "Musae/SimFlux/Messenger/DetectorMessenger.h++"
#include "Musae/SimFlux/SD/LGASD.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include <algorithm>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Musae::SimFlux::inline Action {

DetectorConstruction::DetectorConstruction(bool checkOverlap) :
    PassiveSingleton{this},
    G4VUserDetectorConstruction{},
    fCheckOverlap{checkOverlap} {
    DetectorMessenger::EnsureInstantiation();
}

auto DetectorConstruction::Construct() -> G4VPhysicalVolume* {
    auto fMaterial{std::make_unique<Detector::Definition::Material>()};
    fMaterial->LoadMaterial();

    fWorld = std::make_unique<Detector::Definition::World>();

    const auto& parentMap{Detector::Description::Hierarchy::Instance().ParentMap()};

    if (!parentMap.empty()) {
        // Compute depth from World for topological ordering
        std::map<std::string, int> depth;
        depth["World"] = 0;
        bool changed{true};
        while (changed) {
            changed = false;
            for (const auto& [child, parent] : parentMap) {
                if (depth.contains(parent) && !depth.contains(child)) {
                    depth[child] = depth.at(parent) + 1;
                    changed = true;
                }
            }
        }

        // Verify all components have valid depths (no cycles or missing parents)
        for (const auto& [child, parent] : parentMap) {
            if (!depth.contains(child)) {
                throw std::runtime_error{"Hierarchy error: cannot resolve parent '" + parent + "' for child '" + child + "'. Check for cycles or undefined parents."};
            }
        }

        // Sort by depth (children of World first, then grandchildren, etc.)
        std::vector<std::pair<std::string, std::string>> sorted{parentMap.begin(), parentMap.end()};
        std::ranges::sort(sorted, [&depth](const auto& a, const auto& b) {
            return depth.at(a.first) < depth.at(b.first);
        });

        // Track created components for parent lookup
        std::unordered_map<std::string, Mustard::Detector::Definition::DefinitionBase*> components;
        components["World"] = fWorld.get();

        for (const auto& [child, parent] : sorted) {
            auto* parentDef{components.at(parent)};

            if (child == "Terrain") {
                auto& def{parentDef->NewDaughter<Detector::Definition::Terrain>(fCheckOverlap)};
                components[child] = &def;
            } else if (child == "Model") {
                auto& def{parentDef->NewDaughter<Detector::Definition::Model>(fCheckOverlap)};
                components[child] = &def;
            } else if (child == "LGA") {
                auto& def{parentDef->NewDaughter<Detector::Definition::LGA>(fCheckOverlap)};
                def.RegisterSD("LGAScintillator", new LGASD);
                components[child] = &def;
            } else {
                throw std::runtime_error{"Unknown component in Hierarchy: '" + child + "'. Valid components are: Terrain, Model, LGA."};
            }
        }
    } else {
        // Backward compatibility: no Hierarchy section in YAML
        Mustard::PrintWarning("Hierarchy section not found in YAML. Using default: Model and LGA as direct daughters of World.");
        [[maybe_unused]] auto& model{fWorld->NewDaughter<Detector::Definition::Model>(fCheckOverlap)};
        auto& lga{fWorld->NewDaughter<Detector::Definition::LGA>(fCheckOverlap)};
        lga.RegisterSD("LGAScintillator", new LGASD);
    }

    return fWorld->PhysicalVolume();
}

} // namespace Musae::SimFlux::inline Action
