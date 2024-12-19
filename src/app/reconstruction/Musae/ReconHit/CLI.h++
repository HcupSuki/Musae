#pragma once

#include "Mustard/Env/CLI/AnalysisCLI.h++"
#include "Mustard/Env/CLI/Module/InputTreeModule.h++"
#include "Mustard/Env/CLI/Module/ModuleBase.h++"

#include "argparse/argparse.hpp"

namespace Musae::ReconHit {

class CLIModule : public Mustard::Env::CLI::ModuleBase {
public:
    CLIModule(argparse::ArgumentParser& argParser);

    auto DrawHistogram() const -> auto { return ArgParser()["-h"] == true; }
};

class CLI : public Mustard::Env::CLI::AnalysisCLI<Mustard::Env::CLI::InputTreeModule<"--tree", "-t">,
                                                  CLIModule> {};

} // namespace Musae::ReconHit
