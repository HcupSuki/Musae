#pragma once

#include "Musae/Simulation/Digi/LGAFastDigi.h++"
#include "Musae/Simulation/Digitizer/LGADigitizerBase.h++"

namespace Musae::inline Simulation::inline Digitizer {

class LGAFastDigitizer : public LGADigitizerBase {
public:
    LGAFastDigitizer();

    virtual auto Digitize() -> void override;

    auto DigiCollection() const -> const auto& { return *fDigiCollection; }

private:
    LGAFastDigiCollection* fDigiCollection;
};

} // namespace Musae::inline Simulation::inline Digitizer
