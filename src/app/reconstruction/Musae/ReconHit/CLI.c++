#include "Musae/ReconHit/CLI.h++"

namespace Musae::ReconHit {

CLIModule::CLIModule(argparse::ArgumentParser& argParser) :
    ModuleBase{argParser} {
    ArgParser()
        .add_argument("-h", "--draw-hist")
        .flag()
        .help("Draw hit histogram.");
}

} // namespace Musae::ReconHit
