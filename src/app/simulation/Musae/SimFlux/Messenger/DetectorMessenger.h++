#pragma once

#include "Mustard/Extension/Geant4X/Interface/DetectorMessenger.h++"

#include "Musae/SimFlux/Action/DetectorConstruction.h++"

namespace Musae::SimFlux::inline Messenger {

class DetectorMessenger final : public Mustard::Geant4X::DetectorMessenger<DetectorMessenger,
                                                                           DetectorConstruction,
                                                                           "SimFlux"> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    DetectorMessenger() = default;
    ~DetectorMessenger() = default;
};

} // namespace Musae::SimFlux::inline Messenger
