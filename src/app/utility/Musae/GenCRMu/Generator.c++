#include "Musae/GenCRMu/CLI.h++"
#include "Musae/GenCRMu/Generator.h++"

#include "Mustard/Utility/Print.h++"
#include "Mustard/Utility/VectorCast.h++"

#include "TF2.h"

#include "G4Event.hh"
#include "G4PrimaryParticle.hh"
#include "G4PrimaryVertex.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"

#include "muc/math"

#include "fmt/core.h"

#include <cmath>
#include <functional>
#include <utility>

namespace Musae::GenCRMu {

Generator::Generator(const CLI& cli) :
    fEcoMug{},
    fBias{std::make_unique<TF2>("Bias", cli.BiasFormulaForTF2().c_str(), cli.MinMomentum(), cli.MaxMomentum(), 0, cli.MaxZenith())},
    fBiasMax{fBias->GetMaximum()},
    fInformation{fmt::format("Primary-event z coordinate:           {:.6} m\n"
                             "Target half sphere center coordinate: [{:.6}, {:.6}, {:.6}] m\n"
                             "Target half sphere radius:            {:.6} m\n"
                             "Bias formula:                         {}\n"
                             "Momentum range:                       {:.6} -- {:.6} GeV/c\n"
                             "Incoming zenith angle range:          0 -- {:.6} degree\n",
                             cli.PrimaryZ() / m,
                             cli.TargetCenter()[0] / m, cli.TargetCenter()[1] / m, cli.TargetCenter()[2] / m,
                             cli.TargetRadius() / m,
                             cli.BiasFormula(),
                             cli.MinMomentum() / GeV, cli.MaxMomentum() / GeV,
                             cli.MaxZenith() / deg)} {
    fEcoMug.UseHSphere();
    fEcoMug.HSphereCenterPosition(Mustard::VectorCast<G4ThreeVector>(cli.TargetCenter()));
    fEcoMug.HSphereRadius(cli.TargetRadius());
    fEcoMug.MaxTheta(cli.MaxZenith());
    fEcoMug.MinMomentum(cli.MinMomentum());
    fEcoMug.MaxMomentum(cli.MaxMomentum());
    Mustard::MasterPrint("{}", fInformation);
}

Generator::~Generator() = default;

auto Generator::operator()() -> std::unique_ptr<G4Event> {
    std::unique_ptr<G4Event> event;
    double probability;
    const auto rng{G4Random::getTheEngine()};
    do {
        event = std::make_unique<G4Event>();
        fEcoMug.GeneratePrimaryVertex(event.get());
        const auto primary{event->GetPrimaryVertex()->GetPrimary()};
        probability = fBias->Eval(primary->GetTotalMomentum() / GeV,
                                  std::acos(-primary->GetMomentumDirection().z()) / deg) /
                      fBiasMax;
    } while (rng->flat() >= probability);
    event->GetPrimaryVertex()->SetWeight(1 / probability);
    return event;
}

} // namespace Musae::GenCRMu
