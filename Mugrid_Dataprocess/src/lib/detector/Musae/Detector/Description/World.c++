#include "Musae/Detector/Description/World.h++"

#include "Mustard/Utility/LiteralUnit.h++"

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Length;

World::World() :
    DescriptionBase{"World"},
    fHalfXExtent{1000_km},
    fHalfYExtent{1000_km},
    fHalfZExtent{100_km} {}

auto World::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fHalfXExtent, "HalfXExtent");
    ImportValue(node, fHalfYExtent, "HalfYExtent");
    ImportValue(node, fHalfZExtent, "HalfZExtent");
}

auto World::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fHalfXExtent, "HalfXExtent");
    ExportValue(node, fHalfYExtent, "HalfYExtent");
    ExportValue(node, fHalfZExtent, "HalfZExtent");
}

} // namespace Musae::Detector::Description
