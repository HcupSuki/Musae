#pragma once

#include "Musae/GenCRMu/CRMuEvent.h++"

#include "Mustard/Data/Tuple.h++"
#include "Mustard/Extension/Geant4X/Generator/EcoMugCosmicRayMuon.h++"
#include "Mustard/Utility/LiteralUnit.h++"

#include <memory>
#include <string>
#include <utility>

class G4Event;
class TF2;

namespace Musae::GenCRMu {

using namespace Mustard::LiteralUnit::NumberFlux;

class CLI;

class Generator {
public:
    Generator(const CLI& cli);
    ~Generator();

    auto operator()() -> std::unique_ptr<G4Event>;

    auto Information() const -> auto { return fInformation; }
    auto EstimatedTime(double nMuon, double hFlux = 129_m_2_s_1) -> auto { return fEcoMug.EstimatedTime(nMuon, hFlux); }

private:
    Mustard::Geant4X::EcoMugCosmicRayMuon fEcoMug;
    std::unique_ptr<TF2> fBias;
    double fBiasMax;
    std::string fInformation;
};

} // namespace Musae::GenCRMu
