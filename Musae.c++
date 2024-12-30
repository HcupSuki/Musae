#include "Musae/GenCRMu/GenCRMu.h++"
#include "Musae/ReconLGA/ReconLGA.h++"
#include "Musae/SimFlux/SimFlux.h++"
#include "Musae/VisLGA/VisLGA.h++"

#include "Mustard/Application/SubprogramLauncher.h++"

auto main(int argc, char* argv[]) -> int {
    Mustard::Application::SubprogramLauncher subprogram;
    subprogram.AddSubprogram<Musae::GenCRMu::GenCRMu>();
    subprogram.AddSubprogram<Musae::ReconLGA::ReconLGA>();
    subprogram.AddSubprogram<Musae::SimFlux::SimFlux>();
    subprogram.AddSubprogram<Musae::VisLGA::VisLGA>();
    return subprogram.LaunchMain(argc, argv);
}
