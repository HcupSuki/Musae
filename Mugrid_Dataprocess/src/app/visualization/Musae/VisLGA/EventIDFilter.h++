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
