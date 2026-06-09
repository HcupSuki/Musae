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

#include <string>
#include <string_view>
#include <vector>

namespace Musae::VisLGA {

class EventIDFilter {
private:
    class Range {
    public:
        Range(std::string_view range);

        auto operator()(int i) const -> auto { return fFirst <= i and i <= fLast; }

    private:
        int fFirst;
        int fLast;
    };

public:
    EventIDFilter(const std::vector<std::string>& filter);

    auto operator()(int i) const -> bool;

private:
    std::vector<Range> fRange;
};

} // namespace Musae::VisLGA
