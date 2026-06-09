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
#include "Musae/GenCRMu/CRMuEvent.h++"

#include "G4Event.hh"
#include "G4ParticleDefinition.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"

namespace Musae::GenCRMu {

auto BackProjection(const G4Event& g4Event, double elevation) -> Mustard::Data::Tuple<CRMuEvent> {
    const auto vertex{g4Event.GetPrimaryVertex()};
    const auto primary{vertex->GetPrimary()};
    const auto momentum{primary->GetTotalMomentum()};
    const auto d{primary->GetMomentumDirection()};
    const auto p{momentum * d};
    const auto deltaZ{elevation - vertex->GetZ0()};

    Mustard::Data::Tuple<CRMuEvent> event;
    Get<"t">(event) = vertex->GetT0();
    Get<"x">(event) = vertex->GetX0() + d.x() / d.z() * deltaZ;
    Get<"y">(event) = vertex->GetY0() + d.y() / d.z() * deltaZ;
    Get<"z">(event) = elevation;
    Get<"pdgID">(event)->emplace_back(primary->GetParticleDefinition()->GetPDGEncoding());
    Get<"px">(event)->emplace_back(p.x());
    Get<"py">(event)->emplace_back(p.y());
    Get<"pz">(event)->emplace_back(p.z());
    Get<"w">(event) = vertex->GetWeight();

    return event;
}

} // namespace Musae::GenCRMu
