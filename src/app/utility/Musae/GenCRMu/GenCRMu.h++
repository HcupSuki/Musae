#pragma once

#include "Mustard/Application/Subprogram.h++"

namespace Musae::GenCRMu {

class GenCRMu : public Mustard::Application::Subprogram {
public:
    GenCRMu();
    auto Main(int argc, char* argv[]) const -> int override;
};

} // namespace Musae::GenCRMu
