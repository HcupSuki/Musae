#pragma once

#include "Musae/Data/Event.h++"
#include "Musae/ReconLGA/Type.h++"

#include "muc/ptrvec"

#include <memory>
#include <string_view>
#include <vector>

namespace Musae::ReconLGA {

enum struct ReconstructCRMuMethod {
    LeastChiSquare,
    LeastChiSquareSameWeight
};

auto ParseReconstructCRMuMethod(std::string_view method) -> ReconstructCRMuMethod;

auto ReconstructCRMu(const muc::unique_ptrvec<LGAHit>& eventHit, ReconstructCRMuMethod method) -> std::unique_ptr<CRMuEvent>;

} // namespace Musae::ReconLGA
