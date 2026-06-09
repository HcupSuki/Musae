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

#include "Mustard/Detector/Description/DescriptionBase.h++"

namespace Musae::Detector::Description {

class World final : public Mustard::Detector::Description::DescriptionBase<World> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    World();
    ~World() = default;

public:
    auto HalfXExtent() const -> auto { return fHalfXExtent; }
    auto HalfYExtent() const -> auto { return fHalfYExtent; }
    auto HalfZExtent() const -> auto { return fHalfZExtent; }

    auto HalfXExtent(double val) -> void { fHalfXExtent = val; }
    auto HalfYExtent(double val) -> void { fHalfYExtent = val; }
    auto HalfZExtent(double val) -> void { fHalfZExtent = val; }

private:
    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    double fHalfXExtent;
    double fHalfYExtent;
    double fHalfZExtent;
};

} // namespace Musae::Detector::Description
