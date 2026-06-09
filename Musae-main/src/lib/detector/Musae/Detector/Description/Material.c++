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
#include "Musae/Detector/Description/Material.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Vector/EulerAngles.h"
#include "CLHEP/Vector/Rotation.h"

#include "muc/ctype"
#include "muc/math"

#include "fmt/core.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Energy;
using namespace Mustard::LiteralUnit::Length;
using namespace Mustard::LiteralUnit::Time;

Material::Material() : // clang-format off
    DescriptionWithCacheBase{"Material"}, // clang-format on
    // Geometry
    fPath{this, ""}{}

auto Material::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fPath, "Path");
}

auto Material::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fPath, "Path");
}
} // namespace Musae::Detector::Description
