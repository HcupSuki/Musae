#pragma once

#include "Mustard/Detector/Definition/DefinitionBase.h++"

namespace Musae::Detector::Definition {

class World final : public Mustard::Detector::Definition::DefinitionBase {
private:
    auto Construct(bool checkOverlaps) -> void override;
};

} // namespace Musae::Detector::Definition
