#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"
#include "Mustard/Extension/Geant4X/Generator/FromDataPrimaryGenerator.h++"

#include "G4VUserPrimaryGeneratorAction.hh"

#include <any>

namespace Musae::SimFlux::inline Action {

class PrimaryGeneratorAction final : public Mustard::Env::Memory::PassiveSingleton<PrimaryGeneratorAction>,
                                     public G4VUserPrimaryGeneratorAction {
public:
    PrimaryGeneratorAction();

    auto GeneratePrimaries(G4Event* event) -> void override;
    auto EnablePerfectRangeGeneration(bool val) -> void { fPerfectRangeGeneration = val; }

private:
    Mustard::Geant4X::FromDataPrimaryGenerator fGenerator;

    bool fPerfectRangeGeneration;

    std::any fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux
