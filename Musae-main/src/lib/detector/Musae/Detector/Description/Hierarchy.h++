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

#include <map>
#include <string>

namespace Musae::Detector::Description {

class Hierarchy final : public Mustard::Detector::Description::DescriptionBase<Hierarchy> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    Hierarchy();
    ~Hierarchy() = default;

public:
    auto ParentMap() const -> const std::map<std::string, std::string>& { return fParentMap; }

private:
    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    std::map<std::string, std::string> fParentMap;
};

} // namespace Musae::Detector::Description
