#pragma once

#include "Mustard/Detector/Description/DescriptionBase.h++"

#include <map>
#include <string>

namespace Musae::Detector::Description {

class Hierarchy final : public Mustard::Detector::Description::DescriptionBase<Hierarchy> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    Hierarchy();
    ~Hierarchy() = default;

public:
    auto ParentMap() const -> const std::map<std::string, std::string>& { return fParentMap; }

private:
    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    std::map<std::string, std::string> fParentMap;
};

} // namespace Musae::Detector::Description
