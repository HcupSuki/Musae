#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::VisLGA {

class VisLGA : public Mustard::Application::Subprogram {
public:
    VisLGA();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::VisLGA
