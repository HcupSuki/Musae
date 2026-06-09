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

#include "Mustard/Detector/Description/DescriptionWithCacheBase.h++"

#include "CLHEP/Geometry/Transform3D.h"

#include "muc/array"
#include "muc/btree_map"
#include "muc/hash_map"

#include <unordered_map>
#include <vector>

namespace Musae::Detector::Description {

class Material final : public Mustard::Detector::Description::DescriptionWithCacheBase<Material> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    Material();
    ~Material() = default;

public:
    // Geometry

    auto Path() const -> auto { return *fPath; }

    auto Path(std::string val) -> void { fPath = val; }
private:

    // Digitization

    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    Simple<std::string> fPath;
};

} // namespace Musae::Detector::Description
