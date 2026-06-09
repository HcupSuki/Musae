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

#include "Musae/GenCRMu/CRMuEvent.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Extension/Geant4X/Generator/EcoMugCosmicRayMuon.h++"
#include "Mustard/Utility/LiteralUnit.h++"

#include <memory>
#include <string>
#include <utility>

class G4Event;
class TF2;

namespace Musae::GenCRMu {

using namespace Mustard::LiteralUnit::NumberFlux;

class CLI;

class Generator {
public:
    Generator(const CLI& cli);
    ~Generator();

    auto operator()() -> std::unique_ptr<G4Event>;

    auto Information() const -> auto { return fInformation; }
    auto EstimatedTime(double nMuon, double hFlux = 129_m_2_s_1) -> auto { return fEcoMug.EstimatedTime(nMuon, hFlux); }

private:
    Mustard::Geant4X::EcoMugCosmicRayMuon fEcoMug;
    std::unique_ptr<TF2> fBias;
    double fBiasMax;
    std::string fInformation;
};

} // namespace Musae::GenCRMu
