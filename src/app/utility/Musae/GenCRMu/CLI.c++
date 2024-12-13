#include "Musae/GenCRMu/CLI.h++"

#include "G4SystemOfUnits.hh"

#include "fmt/core.h"

#include <cassert>
#include <string>
#include <vector>

namespace Musae::GenCRMu {

using namespace std::string_literals;

CLIModule::CLIModule(argparse::ArgumentParser& argParser) :
    ModuleBase{argParser} {
    ArgParser()
        .add_argument("n")
        .scan<'i', long long>()
        .help("Number of events to generate.");

    ArgParser()
        .add_argument("-o", "--output")
        .required()
        .help("Output file path.");
    ArgParser()
        .add_argument("-m", "--output-mode")
        .default_value("NEW"s)
        .nargs(1)
        .help("Output file creation mode.");

    ArgParser()
        .add_argument("-h", "--primary-z")
        .scan<'g', double>()
        .required()
        .help("Z coordinate of primary muons (m).");
    ArgParser()
        .add_argument("-t", "--target-center")
        .nargs(3)
        .scan<'g', double>()
        .required()
        .help("Position of the target half sphere (m).");
    ArgParser()
        .add_argument("-r", "--target-radius")
        .scan<'g', double>()
        .required()
        .help("Radius of the target half sphere (m).");

    ArgParser()
        .add_argument("-b", "--bias")
        .default_value("1"s)
        .nargs(1)
        .help("Muon momentum and incoming zenith angle bias function (acceptance function, e.g. '(1-exp(-p[GeV/c]/50)))*exp(-(t[deg]/30)^2)'.");

    ArgParser()
        .add_argument("-p", "--min-momentum")
        .scan<'g', double>()
        .default_value(0.01)
        .nargs(1)
        .help("Minimum muon momentum (GeV/c).");
    ArgParser()
        .add_argument("-q", "--max-momentum")
        .scan<'g', double>()
        .default_value(1000.)
        .nargs(1)
        .help("Maximum muon momentum (GeV/c).");
    ArgParser()
        .add_argument("-z", "--max-zenith")
        .scan<'g', double>()
        .default_value(90.)
        .nargs(1)
        .help("Maximum muon incoming zenith angle (degree).");
}

auto CLIModule::TargetCenter() const -> muc::array3d {
    auto var{ArgParser().get<std::vector<double>>("-t")};
    assert(var.size() == 3);
    return {var.at(0) * m, var.at(1) * m, var.at(2) * m};
}

auto CLIModule::BiasFormulaForTF2() const -> std::string {
    constexpr auto Substitute{
        [](std::string& formula, std::string_view oldVar, std::string_view newVar) {
            for (auto i{formula.find(oldVar)}; i != std::string::npos; i = formula.find(oldVar)) {
                formula.replace(i, oldVar.length(), newVar);
            }
        }};
    auto formula{BiasFormula()};
    Substitute(formula, "p[GeV/c]", "x");
    Substitute(formula, "t[deg]", "y");
    return formula;
}

} // namespace Musae::GenCRMu
