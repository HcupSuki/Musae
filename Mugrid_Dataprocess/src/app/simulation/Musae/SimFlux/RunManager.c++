#include "Musae/SimFlux/Action/ActionInitialization.h++"
#include "Musae/SimFlux/Action/DetectorConstruction.h++"
#include "Musae/SimFlux/Analysis.h++"
#include "Musae/SimFlux/PhysicsList.h++"
#include "Musae/SimFlux/RunManager.h++"

#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Utility/LiteralUnit.h++"

#include "muc/utility"

namespace Musae::SimFlux {

RunManager::RunManager() :
    MPIRunManager{},
    fAnalysis{std::make_unique_for_overwrite<Analysis>()} {
    SetUserInitialization(new PhysicsList);
    SetUserInitialization(new DetectorConstruction{Mustard::Env::VerboseLevelReach<'I'>()});
    SetUserInitialization(new ActionInitialization);
}

RunManager::~RunManager() = default;

} // namespace Musae::SimFlux
