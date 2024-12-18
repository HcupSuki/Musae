#pragma once

#include "Mustard/Env/Memory/PassiveSingleton.h++"

#include "FTFP_BERT.hh"

#include <any>

namespace Musae::SimFlux {

class PhysicsList final : public Mustard::Env::Memory::PassiveSingleton<PhysicsList>,
                          public FTFP_BERT {
public:
    PhysicsList();
    ~PhysicsList();

    auto NonMuonProcessActivation(bool active) -> void;

private:
    std::any fPhysicsMessengerRegister;
};

} // namespace Musae::SimFlux
