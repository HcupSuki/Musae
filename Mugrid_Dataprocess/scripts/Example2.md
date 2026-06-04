# Example 2 — Multi-Position Terrain Muography & 3D Reconstruction

> **Reference guide, not an executable script.** Copy and run each command individually from the `build/` directory. SimFlux (Step 2) requires manual YAML and macro edits for each position and cannot be automated.

## Scene Description

![SimBox Scene](pic/SimBox_M-H_Scene_1_process_v2.png)

Five muon detectors are placed inside a world box composed of rocks with a uniform density of 2.6 g/cm³. The geometric structure of each detector is identical to that of the MuGrid-v2 detector, which comprises three layers of plastic scintillators with segmented light guides. Each scintillator layer has a square cross-section of 30 × 30 cm². The three pale pink cones originating from the detectors represent the acceptance cones with an opening angle of 89°.

The world box has dimensions of 300 × 300 × 90 m³. Above the detectors, three boxes are placed:

| Box | Density (g/cm³) | Color | Size |
|-----|-----------------|-------|------|
| Chalcopyrite | 4.2 | Red | 25 × 25 × 25 m³ |
| Air | 1.2 × 10⁻³ | Blue | 25 × 25 × 25 m³ |
| Lead | 11.34 | Green | 25 × 25 × 25 m³ |

The three boxes are separated by 20 m from each other and are positioned 50 m above the detectors.

## Measurement Geometry

Five detector positions (A–E) plus one open-sky reference at origin:

| Position | X (m) | Y (m) | Z (m) |
|----------|-------|-------|-------|
| A | −242.5 | 133.5 | −51.51 |
| B | −197.5 | 133.5 | −51.51 |
| C | −152.5 | 133.5 | −51.51 |
| D | −220.0 | 94.5 | −51.51 |
| E | −175.0 | 94.5 | −51.51 |

## Step 1: GenCRMu — Generate Cosmic-Ray Muons (×6)

2×10⁹ events per position. Biased momentum-zenith spectrum: `exp(−5/p) · exp(−(θ/45)²)`. Minimum muon momentum 3 GeV/c, max zenith 60°.

```bash
# Position A
mpirun ./Musae GenCRMu 2000000000 -h 300 -t -242.5 133.5 -51.513846 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_A_2e9_p3_z60_b5_r3_2_1.root

# Position B
mpirun ./Musae GenCRMu 2000000000 -h 300 -t -197.5 133.5 -51.513846 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_B_2e9_p3_z60_b5_r3_2_16.root

# Position C
mpirun ./Musae GenCRMu 2000000000 -h 300 -t -152.5 133.5 -51.513846 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_C_2e9_p3_z60_b5_r3_2_16.root

# Position D
mpirun ./Musae GenCRMu 2000000000 -h 300 -t -220 94.5 -51.513846 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_D_2e9_p3_z60_b5_r3_2_16.root

# Position E
mpirun ./Musae GenCRMu 2000000000 -h 300 -t -175 94.5 -51.513846 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_E_2e9_p3_z60_b5_r3_2_16.root

# Open-sky reference (at origin)
mpirun ./Musae GenCRMu 2000000000 -h 300 -t 0 0 0 -r 3 \
    -b 'exp(-5/p[GeV/c]-(t[deg]/45)^2)' -p 3 -z 60 \
    -o ../data/GenCRMu/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16.root
```

## Step 2: SimFlux — Simulate Muon Transport (×6)

Each position requires its own SimFlux run. **Manually edit YAML and macro files before each run.**

The macro file (`Example2.mac`) uses three key Geant4 UI commands:

| Macro command | Meaning | Modification needed |
|---|---|---|
| `/Mustard/Detector/Description/Import` | Path to the YAML geometry file | Point to `Example2.yaml` (positions A–E) or `Example2_fs.yaml` (open-sky) |
| `/Mustard/Analysis/FilePath` | Output directory for simulation results | Change for each run to match the AnaOpacity `-i` path (see below) |
| `/Mustard/Generator/FromDataPrimaryGenerator/EventData` | GenCRMu input ROOT files + generator type (`CRMu`) | Change to the corresponding position's GenCRMu output + `CRMu` |

**Optional pre-check:**
```bash
./Musae SimFlux -i ../scripts/vis.mac
```

### For each position A–E

1. Edit `../scripts/Example2.yaml` → `LGA/Position` to match the measurement coordinates (unit: mm). Example for position A:
   ```yaml
   LGA:
     Position: [[-242500, 133500, -51513.846]]
   ```
2. Edit `../scripts/Example2.mac`:
   - `/Mustard/Analysis/FilePath` → e.g. `../data/SimFlux_vis/SimBox_A_2e9_p3_z60_b5_r3_2_1`
   - `/Mustard/Generator/FromDataPrimaryGenerator/EventData` → e.g. `../data/GenCRMu/SimBox_A_2e9_p3_z60_b5_r3_2_1/* CRMu`
3. Run:
   ```bash
   mpirun ./Musae SimFlux ../scripts/Example2.mac
   ```

### For open-sky reference

1. Edit `../scripts/Example2.mac`:
   - `/Mustard/Detector/Description/Import` → `../scripts/Example2_fs.yaml` (flat terrain, air material)
   - `/Mustard/Analysis/FilePath` → `../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16`
   - `/Mustard/Generator/FromDataPrimaryGenerator/EventData` → `../data/GenCRMu/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/* CRMu`
2. Run:
   ```bash
   mpirun ./Musae SimFlux ../scripts/Example2.mac
   ```

Repeat for all 6 configurations.

## Step 3: AnaOpacity — Analyze Opacity (×5)

Compare each position against the open-sky reference.

```bash
# Position A
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/SimBox_A_2e9_p3_z60_b5_r3_2_1/*.root \
    -j ../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/*.root \
    -h 40 20 1.2 180 0 1 30 -m "RECREATE" \
    -o ../data/Recon_output/SimBox/SimBox_A_2e9_p3_z60_b5_r3_2_13.root \
    -w -c ../data/Csv_output/SimBox/SimBox_A_2e9_p3_z60_b5_r3_2_13.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin3.csv -n 1

# Position B
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/SimBox_B_2e9_p3_z60_b5_r3_2_16/*.root \
    -j ../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/*.root \
    -h 40 20 1.2 180 0 1 30 -m "RECREATE" \
    -o ../data/Recon_output/SimBox/SimBox_B_2e9_p3_z60_b5_r3_2_16.root \
    -w -c ../data/Csv_output/SimBox/SimBox_B_2e9_p3_z60_b5_r3_2_16.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin3.csv -n 1

# Position C
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/SimBox_C_2e9_p3_z60_b5_r3_2_16/*.root \
    -j ../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/*.root \
    -h 40 20 1.2 180 0 1 30 -m "RECREATE" \
    -o ../data/Recon_output/SimBox/SimBox_C_2e9_p3_z60_b5_r3_2_21.root \
    -w -c ../data/Csv_output/SimBox/SimBox_C_2e9_p3_z60_b5_r3_2_21.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin3.csv -n 1

# Position D
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/SimBox_D_2e9_p3_z60_b5_r3_2_16/*.root \
    -j ../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/*.root \
    -h 40 20 1.2 180 0 1 30 -m "RECREATE" \
    -o ../data/Recon_output/SimBox/SimBox_D_2e9_p3_z60_b5_r3_2_21.root \
    -w -c ../data/Csv_output/SimBox/SimBox_D_2e9_p3_z60_b5_r3_2_21.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin3.csv -n 1

# Position E
./Musae AnaOpacity \
    -i ../data/SimFlux_vis/SimBox_E_2e9_p3_z60_b5_r3_2_16/*.root \
    -j ../data/SimFlux_vis/SimBox_OpenSky_2e9_p3_z60_b5_r3_2_16/*.root \
    -h 40 20 1.2 180 0 1 30 -m "RECREATE" \
    -o ../data/Recon_output/SimBox/SimBox_E_2e9_p3_z60_b5_r3_2_21.root \
    -w -c ../data/Csv_output/SimBox/SimBox_E_2e9_p3_z60_b5_r3_2_21.csv \
    -s -f ../data/Flux_model/Survival_to_Ecut_table_pmin3.csv -n 1
```

## Next: 3D Reconstruction (Mugrid_Reconstract)

The 5 CSV files above are the input for 3D density reconstruction. Copy them:

```bash
cp ../data/Csv_output/SimBox/SimBox_*_2e9_*.csv ../../Mugrid_Reconstract/input/
```

Then follow [MANUAL.md](../../MANUAL.md) Section 5, Steps 4–5.
