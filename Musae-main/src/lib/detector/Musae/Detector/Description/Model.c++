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
#include "Musae/Detector/Description/Model.h++"

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

Model::Model() : // clang-format off
    DescriptionWithCacheBase{"Model"}, // clang-format on
    // Geometry
    fPath{this, ""},
    fPosition{this, {0, 0, 0}},
    fModelWidthX{this, 10_m},
    fModelWidthY{this, 10_m},
    fModelHeightZ{this, 10_m},
    fMaterial{this, ""}{}

auto Model::Transform() const -> HepGeom::Transform3D {
    return HepGeom::Translate3D{Position()[0], Position()[1], Position()[2]};
}


auto Model::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fPath, "Path");
    ImportValue(node, fPosition, "Position");
    ImportValue(node, fModelWidthX, "ModelWidthX");
    ImportValue(node, fModelWidthY, "ModelWidthY");
    ImportValue(node, fModelHeightZ, "ModelHeightZ");
    ImportValue(node, fMaterial, "Material");

}

auto Model::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fPath, "Path");
    ExportValue(node, fPosition, "Position");
    ExportValue(node, fModelWidthX, "ModelWidthX");
    ExportValue(node, fModelWidthY, "ModelWidthY");
    ExportValue(node, fModelHeightZ, "ModelHeightZ");
    ExportValue(node, fMaterial, "Material");
}

} // namespace Musae::Detector::Description
