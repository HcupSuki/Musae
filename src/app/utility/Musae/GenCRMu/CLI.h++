#pragma once

#include "Mustard/Env/CLI/CLI.h++"
#include "Mustard/Env/CLI/Module/BasicModule.h++"
#include "Mustard/Env/CLI/Module/ModuleBase.h++"
#include "Mustard/Env/CLI/Module/MonteCarloModule.h++"

#include "CLHEP/Units/SystemOfUnits.h"

#include "argparse/argparse.hpp"

#include "muc/array"

#include <filesystem>
#include <string>

namespace Musae::GenCRMu {

class CLIModule : public Mustard::Env::CLI::ModuleBase {
public:
    CLIModule(argparse::ArgumentParser& argParser);

    auto PrimaryZ() const -> auto { return ArgParser().get<double>("-h") * CLHEP::m; }
    auto TargetCenter() const -> muc::array3d;
    auto TargetRadius() const -> auto { return ArgParser().get<double>("-r") * CLHEP::m; }

    auto BiasFormula() const -> auto { return ArgParser().get("-b"); }
    auto BiasFormulaForTF2() const -> std::string;

    auto MinMomentum() const -> auto { return ArgParser().get<double>("-p") * CLHEP::GeV; }
    auto MaxMomentum() const -> auto { return ArgParser().get<double>("-q") * CLHEP::GeV; }
    auto MaxZenith() const -> auto { return ArgParser().get<double>("-z") * CLHEP::deg; }
};

class CLI : public Mustard::Env::CLI::CLI<Mustard::Env::CLI::BasicModule,
                                          Mustard::Env::CLI::MonteCarloModule,
                                          CLIModule> {};

} // namespace Musae::GenCRMu
