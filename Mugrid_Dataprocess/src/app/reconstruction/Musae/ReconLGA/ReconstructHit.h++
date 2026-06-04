#pragma once

#include "Musae/Data/Digi.h++"
#include "Musae/Data/Hit.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/hash_map"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

enum struct ReconstructHitMethod {
    SLinTW,
    OLinTW,
    EW,
    NEW,
    SLinTWCombEW,
    SLinTWCombNEW,
    SPowTW,
    SPowTWCombSymFil,
    EWCombSymFil,
    NEWCombSymFil
};

auto ParseReconstructHitMethod(std::string_view method) -> ReconstructHitMethod;

auto ReconstructHit(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiDataOri,
                    int hitID, ReconstructHitMethod method) -> std::unique_ptr<LGAHit>;

} // namespace Musae::ReconLGA
