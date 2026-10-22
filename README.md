# Musae — MUon Scattering and Absorption tomography simulation infrastructurE

Muon-based imaging and 3D density reconstruction framework with two main components:

- **Musae-main** (C++/Geant4): End-to-end cosmic-ray muon simulation pipeline — event generation (EcoMug), detector transport (SimFlux), opacity analysis (AnaOpacity), and 2D projection mapping (Projection). Also processes real experimental LGA detector data.
- **MuCT** (Python): 3D density and terrain inversion from muon opacity data using linear solvers (L-BFGS-B) and MCMC sampling (Metropolis-Hastings). Includes interactive 3D visualization with Plotly/PyVista and a PyQt5/VTK desktop GUI (`MuCT/mugui`) for density, terrain, and opacity inspection.

For detailed usage instructions, workflows, and parameter references, see **[MANUAL.md](MANUAL.md)**.

## Quick Start

### 1. Build Musae-main

**Required system dependencies:**
- CMake ≥ 3.21
- GCC ≥ 13 (or Clang ≥ 17) for C++20 `<format>` support
- MPI ≥ 3.1: `sudo apt install libopenmpi-dev`
- Eigen ≥ 3.4.0: `sudo apt install libeigen3-dev`
- Geant4 ≥ 11.0.0 with GDML enabled (see [install guide](https://geant4-userdoc.web.cern.ch/UsersGuides/InstallationGuide/html/))
- ROOT ≥ 6.30.00 (see [install guide](https://root.cern.ch/install/))

**For Ubuntu/Debian, one-line dependency install (plus Geant4 and ROOT):**
```bash
sudo apt install build-essential cmake libopenmpi-dev libeigen3-dev \
    libx11-dev libxext-dev libxpm-dev libgl-dev libglu-dev \
    qt6-base-dev qt6-base-dev-tools libxerces-c-dev libexpat1-dev \
    zlib1g-dev libfreetype-dev
```

**Build:**
```bash
cd Musae-main
mkdir -p build && cd build
cmake .. -DMUSAE_BUILTIN_MUSTARD=ON -DCMAKE_POLICY_VERSION_MINIMUM=3.5
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

### 4. Launch the visualization GUI (optional)

```bash
cd MuCT
muCT_local/bin/python mugui/main.py
```

The GUI requires PyQt5 and VTK; they are included in `MuCT/requirements_muCT.txt`. If you use an existing environment, install them with `pip install PyQt5 vtk`.

## MuCT Visualization GUI

`MuCT/mugui` is a PyQt5 + VTK desktop application for inspecting MuCT reconstruction products without editing notebooks. It provides three independent tabs:

| Tab | Input | Main view |
|---|---|---|
| **Density Reconstruction** | `.npy` + same-name `.txt`, or CSV with `rho` | Voxel density blocks, threshold/transparency filters, measurement points, auxiliary geometry, true-terrain shell |
| **Terrain Reconstruction** | `.npy` + `.txt`, or CSV with `X,Y,height` | Height surface with median/Gaussian filtering, `-55` holes, colored true-terrain overlay; no tunnel or detector geometry |
| **Opacity Distribution** | AnaOpacity CSV with `u,v,w,bincontent,error` (multi-select) | Angular bins drawn as radial columns from each detector, colored by mass thickness |

Key features:

- **Scene presets** (`mugui/scene_presets.json`): SimBox/Tianqin detector positions, view ranges, auxiliary geometry, and true-terrain CSV paths. Presets are matched by file name; an unknown data set such as RuYuan does not inherit a previous preset and defaults its detector origin to `(0,0,0)`.
- **Per-tab true terrain**: each tab has its own **Load true terrain CSV...** button in the Scene area, so different reconstruction methods can use different DEMs. A missing preset terrain file produces a visible warning instead of a silent failure.
- **Custom detectors**: select **Custom** in the scene preset, enter one `name x y z` per line, then click **Apply detector positions**. The detector markers, opacity columns, and camera center update only after this confirmation.
- **Opacity columns**: each angular bin has an end cap at radius `r = r_min + (r_max-r_min)*(v/vmax)^gamma`, four side faces from the detector to the cap, and four corner lines. Sides are semi-transparent so the column height remains readable. Detector markers and labels are rendered in an overlay layer and stay visible.
- **Interaction**: left-drag rotate, wheel zoom, middle-drag pan, **Reset camera**, and PNG screenshots at 1x/2x/4x.

Data conventions:

- Density `.npy` files must have a matching `.txt` JSON file containing `N_X,N_Y,N_Z` and `X/Y/Z_MIN/MAX`; `-1` marks invalid voxels.
- Terrain `.npy` files must have a matching `.txt` with `N_X,N_Y` and `X/Y_MIN/MAX`; `-55` marks invalid cells.
- Opacity CSVs come from `AnaOpacity`; the detector letter is parsed from the file name when possible (for example `SimBox_A_...` or `TianqinNewPbCube_A_...`).

See the **[GUI chapter in MANUAL.md](MANUAL.md#33-desktop-visualization-gui-mugui)** for controls, presets, and troubleshooting.

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
│   ├── requirements_muCT.txt     # Python dependencies (includes optional GUI deps)
│   ├── mugui/                    # PyQt5 + VTK desktop visualization GUI
│   │   ├── main.py               # Entry point (Qt library path + CJK font setup)
│   │   ├── app.py                # Main window, tabs, toolbar, status bar
│   │   ├── widgets.py            # Control panels and custom widgets
│   │   ├── loaders.py            # Format detection and data loading
│   │   ├── render.py             # VTK pipelines and the three view classes
│   │   ├── scene_presets.json    # SimBox/Tianqin scene presets
│   │   └── fonts/                # Bundled Noto Sans CJK SC (SIL OFL 1.1)
│   └── input/Terrain/            # Terrain elevation CSV data
└── .gitignore
```

## Key References

- [MANUAL.md](MANUAL.md) — Full user manual with workflow examples, parameter tables, and the GUI chapter
- [GUI chapter](MANUAL.md#33-desktop-visualization-gui-mugui) — Launching and using the MuCT desktop visualization GUI
- Example workflows: [`scripts/Example1.md`](Musae-main/scripts/Example1.md), [`scripts/Example2.md`](Musae-main/scripts/Example2.md)
- Mustard framework: Geant4-based simulation toolkit
- EcoMug: Cosmic-ray muon generator used by GenCRMu

## License

This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version. See [LICENSE](LICENSE) for the full text.

Copyright (C) 2026 Musae developers
