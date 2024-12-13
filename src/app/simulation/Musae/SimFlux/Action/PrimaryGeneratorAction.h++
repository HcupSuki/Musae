#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"
#include "Mustard/Extension/Geant4X/Generator/FromDataPrimaryGenerator.h++"

#include "G4VUserPrimaryGeneratorAction.hh"

namespace Musae::SimFlux::inline Action {

class PrimaryGeneratorAction final : public Mustard::Env::Memory::PassiveSingleton<PrimaryGeneratorAction>,
                                     public G4VUserPrimaryGeneratorAction {
public:
    auto GeneratePrimaries(G4Event* event) -> void override;

private:
    Mustard::Geant4X::FromDataPrimaryGenerator fGenerator;
};

} // namespace Musae::SimFlux
