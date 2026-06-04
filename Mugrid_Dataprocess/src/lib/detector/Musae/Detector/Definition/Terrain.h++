#pragma once

#include "Mustard/Detector/Definition/DefinitionBase.h++"

#include <memory>

namespace Musae::Detector::Definition {

class Terrain final : public Mustard::Detector::Definition::DefinitionBase {
private:
    auto Construct(bool checkOverlaps) -> void override;
};

} // namespace Musae::Detector::Definition
