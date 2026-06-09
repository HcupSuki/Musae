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
#include "Musae/SimFlux/Action/EventAction.h++"
#include "Musae/SimFlux/Analysis.h++"

namespace Musae::SimFlux::inline Action {

EventAction::EventAction() :
    PassiveSingleton{this} {}

auto EventAction::EndOfEventAction(const G4Event*) -> void {
    Analysis::Instance().EventEndAction();
}

} // namespace Musae::SimFlux::inline Action
