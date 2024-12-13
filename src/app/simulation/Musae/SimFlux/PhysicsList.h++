#pragma once

#include "Musae/SimFlux/Messenger/PhysicsMessenger.h++"

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "FTFP_BERT.hh"

namespace Musae::SimFlux {

class PhysicsList final : public Mustard::Env::Memory::PassiveSingleton<PhysicsList>,
                          public FTFP_BERT {
public:
    PhysicsList();

    auto NonMuonProcessActivation(bool active) -> void;

private:
    PhysicsMessenger::Register<PhysicsList> fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux
