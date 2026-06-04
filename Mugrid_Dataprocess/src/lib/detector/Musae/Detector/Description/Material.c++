#include "Musae/Detector/Description/Material.h++"

#include "Mustard/Utility/LiteralUnit.h++"
#include "Mustard/Utility/PrettyLog.h++"

#include "CLHEP/Vector/EulerAngles.h"
#include "CLHEP/Vector/Rotation.h"

#include "muc/ctype"
#include "muc/math"

#include "fmt/core.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace Musae::Detector::Description {

using namespace Mustard::LiteralUnit::Energy;
using namespace Mustard::LiteralUnit::Length;
using namespace Mustard::LiteralUnit::Time;

Material::Material() : // clang-format off
    DescriptionWithCacheBase{"Material"}, // clang-format on
    // Geometry
    fPath{this, ""}{}

auto Material::ImportAllValue(const YAML::Node& node) -> void {
    ImportValue(node, fPath, "Path");
}

auto Material::ExportAllValue(YAML::Node& node) const -> void {
    ExportValue(node, fPath, "Path");
}
} // namespace Musae::Detector::Description
