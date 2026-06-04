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
