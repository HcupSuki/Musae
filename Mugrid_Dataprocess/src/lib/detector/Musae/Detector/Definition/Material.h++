#pragma once

#include "Mustard/Utility/NonMoveableBase.h++"

namespace Musae::Detector::Definition {

class Material final : public Mustard::Utility::NonMoveableBase {
public:
    virtual ~Material() = default;

    auto LoadMaterial() -> void;
};
} // namespace Musae::Detector::Definition
