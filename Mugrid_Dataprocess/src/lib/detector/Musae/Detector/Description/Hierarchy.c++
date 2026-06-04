#include "Musae/Detector/Description/Hierarchy.h++"

namespace Musae::Detector::Description {

Hierarchy::Hierarchy() :
    DescriptionBase{"Hierarchy"},
    fParentMap{} {}

auto Hierarchy::ImportAllValue(const YAML::Node& node) -> void {
    fParentMap.clear();
    for (auto it = node.begin(); it != node.end(); ++it) {
        fParentMap[it->first.as<std::string>()] = it->second.as<std::string>();
    }
}

auto Hierarchy::ExportAllValue(YAML::Node& node) const -> void {
    for (const auto& [child, parent] : fParentMap) {
        node[child] = parent;
    }
}

} // namespace Musae::Detector::Description
