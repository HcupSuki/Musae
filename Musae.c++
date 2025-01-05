#include "Musae/AnaOpacity/AnaOpacity.h++"
#include "Musae/GenCRMu/GenCRMu.h++"
#include "Musae/ReconLGA/ReconLGA.h++"
#include "Musae/SimFlux/SimFlux.h++"
#include "Musae/VisLGA/VisLGA.h++"

#include "Mustard/Application/SubprogramLauncher.h++"

auto main(int argc, char* argv[]) -> int {
    Mustard::Application::SubprogramLauncher launcher;
    launcher.AddSubprogram<Musae::AnaOpacity::AnaOpacity>();
    launcher.AddSubprogram<Musae::GenCRMu::GenCRMu>();
    launcher.AddSubprogram<Musae::ReconLGA::ReconLGA>();
    launcher.AddSubprogram<Musae::SimFlux::SimFlux>();
    launcher.AddSubprogram<Musae::VisLGA::VisLGA>();
    return launcher.LaunchMain(argc, argv);
}
