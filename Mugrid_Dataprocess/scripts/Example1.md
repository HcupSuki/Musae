# Example 1 — Concrete Wall Cavity Detection

> **Reference guide, not an executable script.** Copy and run each command individually from the `build/` directory. SimFlux (Step 2) requires manual YAML and macro edits between scenarios and cannot be automated.

## Step 1: GenCRMu — Generate Cosmic-Ray Muon Events

Generate 1×10⁹ CRMu events on a hemisphere (radius 3 m, height 300 m).

| Flag | Value | Description |
|------|-------|-------------|
| `-p` | `0` | Flat momentum spectrum |
| `-z` | `60` | Maximum zenith angle 60° |

```bash
mpirun ./Musae GenCRMu 1000000000 -h 300 -t 0 0 0 -r 3 -p 0 -z 60 \
    -o ../data/GenCRMu/ConcreteWall_test.root -m RECREATE
```

Run `./Musae GenCRMu --help` for more options.

## Step 2: SimFlux — Simulate Muon Transport

Two scenarios are required: wall **with** cavity and wall **without** cavity. Each needs a separate YAML edit, macro edit, and SimFlux run.

The macro file (`Example1.mac`) uses three key Geant4 UI commands:

| Macro command | Meaning | Modification needed |
|---|---|---|
| `/Mustard/Detector/Description/Import` | Path to the YAML geometry file | Edit the YAML to toggle cavity STL on/off (see below) |
| `/Mustard/Analysis/FilePath` | Output directory for simulation results | Change to match the AnaOpacity `-i`/`-j` paths (see below) |
| `/Mustard/Generator/FromDataPrimaryGenerator/EventData` | GenCRMu input ROOT files + generator type (`CRMu`) | Ensure it points to the GenCRMu output from Step 1 |

**Optional pre-check** — visualize geometry before the full run:
```bash
./Musae SimFlux -i ../scripts/vis.mac
# In the visualization window: /run/beamOn 1000
# Close the window afterwards.
```

### Scenario A: Wall WITH cavity

1. Edit `../scripts/Example1.yaml`:
   - **Uncomment** `ConcreteWallWithCubeHole0p7_Unit_mm.stl`
   - **Comment out** `ConcreteWall_Unit_mm.stl`
2. Edit `../scripts/Example1.mac`:
   - Set `/Mustard/Analysis/FilePath` to `../data/SimFlux_vis/ConcreteWall_test`
3. Run:
   ```bash
   mpirun ./Musae SimFlux ../scripts/Example1.mac
   ```
   Output goes to `../data/SimFlux_vis/ConcreteWall_test/`.

### Scenario B: Wall WITHOUT cavity

1. Edit `../scripts/Example1.yaml`:
   - **Comment out** the cavity STL
   - **Uncomment** `ConcreteWall_Unit_mm.stl`
2. Edit `../scripts/Example1.mac`:
   - Set `/Mustard/Analysis/FilePath` to `../data/SimFlux_vis/ConcreteWall_test_noHole`
3. Run:
   ```bash
   mpirun ./Musae SimFlux ../scripts/Example1.mac
   ```
   Output goes to `../data/SimFlux_vis/ConcreteWall_test_noHole/`.

## Step 3: AnaOpacity — Survival Fractions & Energy-Thickness Maps

| Flag | Description |
|------|-------------|
| `-i` | Target data (with cavity) |
| `-j` | Reference data (open-sky / without cavity) |
| `-h` | Binning: azimuth bins, zenith bins, zenith step, ... |
| `-c` | Output CSV for downstream analysis |
| `-f` | Survival-to-energy-range lookup table |

```bash
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/ConcreteWall_test/*.root \
    -j ../data/SimFlux_vis/ConcreteWall_test_noHole/*.root \
    -h 160 80 1.6 180 0 1 20 \
    -m "RECREATE" \
    -o ../data/Recon_output/ConcreteWall/ConcreteWall_test.root \
    -w -c ../data/Csv_output/ConcreteWall/ConcreteWall_test.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin0.csv
```

## Step 4: Projection — 2D Back-Projection & Residual Analysis

| Flag | Description |
|------|-------------|
| `-h` | Grid: X bins, Y bins, X range, Y range, residual limits, cut |
| `-a` | Detector angular acceptance factor |
| `-e` | Rotation angles of the projection plane (Z-Y-Z Euler angles in degrees) |

```bash
./Musae Projection \
    -i ../data/SimFlux_vis/ConcreteWall_test/*.root \
    -j ../data/SimFlux_vis/ConcreteWall_test_noHole/*.root \
    -h 50 30 -5000 5000 -1000 5000 5 -5 0 \
    -m "RECREATE" \
    -o ../data/Recon_output/ConcreteWall/ConcreteWall_test_Projection.root \
    -a 0.712132 -e 90 90 90 -w
```

## Euler Angle Convention

When transforming detector-local to world coordinates:

- **World frame**: Y = north, X = east, Z = up
- **Detector frame**: (x₁, y₁, z₁)
- **Sequence**: intrinsic Z-Y-Z (Z → Y' → Z'')
- **Direction**: world → detector (right-hand rule = positive)
