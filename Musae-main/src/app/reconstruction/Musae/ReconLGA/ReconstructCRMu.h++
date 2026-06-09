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

#include "Musae/Data/Event.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/ptrvec"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

enum struct ReconstructCRMuMethod {
    LeastChiSquare,
    LeastChiSquareSameWeight
};

auto ParseReconstructCRMuMethod(std::string_view method) -> ReconstructCRMuMethod;

auto ReconstructCRMu(const muc::unique_ptrvec<LGAHit>& eventHit, ReconstructCRMuMethod method) -> std::unique_ptr<CRMuEvent>;

} // namespace Musae::ReconLGA
