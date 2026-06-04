#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::SimFlux {

class SimFlux : public Mustard::Application::Subprogram {
public:
    SimFlux();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::SimFlux
