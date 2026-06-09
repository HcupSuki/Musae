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
#include "Musae/VisLGA/EventIDFilter.h++"

#include "Mustard/Utility/PrettyLog.h++"

#include "fmt/core.h"

#include <algorithm>
#include <stdexcept>

namespace Musae::VisLGA {

using namespace std::string_view_literals;

EventIDFilter::Range::Range(std::string_view range) :
    fFirst{},
    fLast{} {
    const auto dotDot{range.find_first_of("..")};
    try {
        fFirst = std::stoi(std::string{range.substr(0, dotDot)});
        fLast = std::stoi(std::string{range.substr(dotDot + ".."sv.length())});
    } catch (const std::invalid_argument&) {
        Mustard::Throw<std::invalid_argument>(fmt::format("Cannot parse '{}'", range));
    } catch (const std::out_of_range&) {
        Mustard::Throw<std::invalid_argument>(fmt::format("Range '{}' itself out of range", range));
    }
}

EventIDFilter::EventIDFilter(const std::vector<std::string>& filter) :
    fRange{} {
    for (auto&& f : filter) {
        fRange.emplace_back(f);
    }
}

auto EventIDFilter::operator()(int i) const -> bool {
    return std::ranges::any_of(fRange, [i](auto&& r) { return r(i); });
}

} // namespace Musae::VisLGA
