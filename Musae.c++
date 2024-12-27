#include "Musae/GenCRMu/GenCRMu.h++"
#include "Musae/ReconLGA/ReconLGA.h++"
#include "Musae/SimFlux/SimFlux.h++"

#include "Mustard/Application/SubprogramLauncher.h++"

auto main(int argc, char* argv[]) -> int {
    Mustard::Application::SubprogramLauncher subprogram;
    subprogram.AddSubprogram<Musae::GenCRMu::GenCRMu>();
    subprogram.AddSubprogram<Musae::ReconLGA::ReconLGA>();
    subprogram.AddSubprogram<Musae::SimFlux::SimFlux>();
    return subprogram.LaunchMain(argc, argv);
}
