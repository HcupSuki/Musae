# Musae — MUon Scattering and Absorption tomography simulation infrastructurE

Muon-based imaging and 3D density reconstruction framework with two main components:

- **Musae-main** (C++/Geant4): End-to-end cosmic-ray muon simulation pipeline — event generation (EcoMug), detector transport (SimFlux), opacity analysis (AnaOpacity), and 2D projection mapping (Projection). Also processes real experimental LGA detector data.
- **MuCT** (Python): 3D density inversion from muon opacity data using linear solvers (L-BFGS-B) and MCMC sampling (Metropolis-Hastings). Includes interactive 3D visualization with Plotly and PyVista, plus terrain model integration.

For detailed usage instructions, workflows, and parameter references, see **[MANUAL.md](MANUAL.md)**.

## Quick Start

### 1. Build Musae-main

Requires CMake ≥ 3.21, **GCC ≥ 13** (or **Clang ≥ 17**) for C++20 `<format>` support, ROOT, Geant4, OpenMPI.

```bash
cd Musae-main
mkdir -p build && cd build
cmake .. -DMUSAE_BUILTIN_MUSTARD=ON
make -j$(nproc)
```

### 2. Set up Python environment

Requires Python ≥ 3.9.

```bash
cd MuCT
python3 -m venv muCT_local
source muCT_local/bin/activate
pip install -r requirements_muCT.txt
```

### 3. Run an example

From the `build/` directory, follow the step-by-step workflow in [`../scripts/Example1.md`](Musae-main/scripts/Example1.md) (single-position cavity detection) or [`../scripts/Example2.md`](Musae-main/scripts/Example2.md) (multi-position terrain muography with 3D reconstruction). These are reference guides — copy and run commands individually, as SimFlux requires manual YAML edits between runs.

## Project Structure

```
MuCT_Project/
├── LICENSE                       # GNU General Public License v3
├── MANUAL.md                     # Detailed user manual and workflow guide
├── README.md                     # This file
├── Musae-main/                   # C++ simulation & analysis engine (MuSAE)
│   ├── src/
│   │   ├── app/                  # Application modules
│   │   │   ├── analysis/Musae/   #   AnaOpacity, Projection
│   │   │   ├── reconstruction/   #   ReconLGA (experimental data)
│   │   │   ├── simulation/       #   SimFlux
│   │   │   └── utility/          #   GenCRMu
│   │   └── lib/detector/         # Detector geometry definitions
│   ├── scripts/                  # Workflow reference scripts & config files
│   └── data/                     # Input/output data (gitignored)
│       ├── GenCRMu/              #   Generated muon events (.root)
│       ├── SimFlux_vis/          #   Simulation output
│       ├── Csv_output/           #   AnaOpacity CSV output
│       ├── Flux_model/           #   Energy/survival lookup tables
│       └── Recon_output/         #   Reconstruction-ready ROOT files
├── MuCT/                         # Python 3D reconstruction & visualization
│   ├── Example2_reconstruct.ipynb # 3D density inversion (M-H + L-BFGS-B)
│   ├── Example2_visualize.ipynb  # Interactive 3D density visualization
│   ├── requirements_muCT.txt     # Python dependencies
│   └── input/Terrain/            # Terrain elevation CSV data
└── .gitignore
```

## Key References

- [MANUAL.md](MANUAL.md) — Full user manual with workflow examples and parameter tables
- Example workflows: [`scripts/Example1.md`](Musae-main/scripts/Example1.md), [`scripts/Example2.md`](Musae-main/scripts/Example2.md)
- Mustard framework: Geant4-based simulation toolkit
- EcoMug: Cosmic-ray muon generator used by GenCRMu

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. See [LICENSE](LICENSE) for the full text.

Copyright (C) 2026 Musae developers
