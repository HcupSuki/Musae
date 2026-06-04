#pragma once

#include "Mustard/Detector/Description/DescriptionWithCacheBase.h++"

#include "CLHEP/Geometry/Transform3D.h"

#include "muc/array"
#include "muc/btree_map"
#include "muc/hash_map"

#include <unordered_map>
#include <vector>

namespace Musae::Detector::Description {

class Material final : public Mustard::Detector::Description::DescriptionWithCacheBase<Material> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    Material();
    ~Material() = default;

public:
    // Geometry

    auto Path() const -> auto { return *fPath; }

    auto Path(std::string val) -> void { fPath = val; }
private:

    // Digitization

    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    Simple<std::string> fPath;
};

} // namespace Musae::Detector::Description
