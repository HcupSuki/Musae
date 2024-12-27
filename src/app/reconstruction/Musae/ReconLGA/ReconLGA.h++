#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::ReconLGA {

class ReconLGA : public Mustard::Application::Subprogram {
public:
    ReconLGA();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::ReconLGA
