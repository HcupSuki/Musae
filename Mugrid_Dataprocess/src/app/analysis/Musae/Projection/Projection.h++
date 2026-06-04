#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::Projection {

class Projection : public Mustard::Application::Subprogram {
public:
    Projection();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::Projection
