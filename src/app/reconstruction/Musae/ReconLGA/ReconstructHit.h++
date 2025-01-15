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
    SLinTW1D,
    SLinTW2D,
    EW1D,
    NEW1D,
    SLinTEW1D,
    SLinTNEW1D,
    SLinTTopW1D,
    ETopW1D,
    NETopW1D,
    SLinTETopW1D,
    SLinTNETopW1D,
    SLinTW1DCombEW1D,
    SLinTW1DCombNEW1D,
};

auto ParseReconstructHitMethod(std::string_view method) -> ReconstructHitMethod;

auto ReconstructHit(const muc::flat_hash_map<char, std::vector<LGADigi*>>& digiData,
                    int hitID, ReconstructHitMethod method) -> std::unique_ptr<LGAHit>;

} // namespace Musae::ReconLGA
