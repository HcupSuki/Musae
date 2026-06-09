// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
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
    auto EventNumber() const -> auto { return ArgParser().get<unsigned long long>("n"); }
};

class CLI : public Mustard::Env::CLI::CLI<Mustard::Env::CLI::BasicModule,
                                          Mustard::Env::CLI::MonteCarloModule,
                                          CLIModule> {};

} // namespace Musae::GenCRMu
