#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::AnaOpacity {

class AnaOpacity : public Mustard::Application::Subprogram {
public:
    AnaOpacity();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::AnaOpacity
