// SPDX-License-Identifier: GPL-3.0-or-later
// Musae - MUon Scattering and Absorption tomography simulation infrastructurE
// Copyright (C) 2026 Musae developers
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.
//
#include "Musae/AnaScatter/AnaScatter.h++"
#include "Musae/AnaOpacity/AnaProcess.h++"

#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"
#include "Mustard/Utility/Print.h++"

#include "ROOT/RDataFrame.hxx"
#include "TChain.h"

#include <cmath>
#include <cstdlib>
#include <numbers>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Musae::AnaScatter {

AnaScatter::AnaScatter() :
    Subprogram{"AnaScatter", "Scattering imaging data preprocessing program."} {}

using namespace std::string_literals;

namespace {

struct ScatterEvent {
    int evtID;
    int detID;
    float theta;
    float phi;
    float x;
    float y;
    float z;
    float px;
    float py;
    float pz;
};

} // anonymous namespace

auto AnaScatter::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("-i", "--input")
        .help("Input ROOT file path(s). Supports wildcards.")
        .required()
        .nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--tree")
        .help("TTree path name(s), comma-separated.")
        .default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"})
        .nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-e", "--euler-transform")
        .help("Euler transform angles (Gamma Beta Alpha) in degrees.")
        .default_value(std::vector<double>{0.0, 0.0, 0.0})
        .nargs(3)
        .scan<'g', double>();
    cli->add_argument("-o", "--output")
        .help("Output CSV file path.")
        .required()
        .nargs(1);

    Mustard::Env::BasicEnv env{argc, argv, cli};

    // ---- Validate Euler angles ----
    const auto angleToTurn{cli->get<std::vector<double>>("--euler-transform")};
    if (angleToTurn.size() != 3) {
        throw std::invalid_argument("Euler transform angle must have exactly 3 components (Gamma, Beta, Alpha).");
    }

    // ---- Build tree path string ----
    auto treePaths = cli->get<std::vector<std::string>>("--tree");
    std::string treeStr = treePaths[0];
    for (size_t i = 1; i < treePaths.size(); ++i) {
        treeStr += "," + treePaths[i];
    }

    // ---- Load data via TChain → RDataFrame ----
    std::unique_ptr<TChain> chain = AnaOpacity::LoadData(treeStr,
        cli->get<std::vector<std::string>>("--input"));
    ROOT::RDataFrame rdf(*chain);

    const bool doTransform = (angleToTurn[0] != 0.0 ||
                              angleToTurn[1] != 0.0 ||
                              angleToTurn[2] != 0.0);

    // ---- Process theta/phi (with optional Euler transform) ----
    // Define float-valued columns for theta and phi (original data is double/float)
    auto rdfWithTP = rdf.Define("theta_f", "static_cast<float>(theta)")
                         .Define("phi_f", "static_cast<float>(phi)");

    std::unique_ptr<std::vector<float>> thetaTransformed;
    std::unique_ptr<std::vector<float>> phiTransformed;

    if (doTransform) {
        // Reuse AnaOpacity's Euler transform pipeline: θ,φ → uvw → rotate → θ',φ'
        auto* uvw = AnaOpacity::thetaphi2uvw(rdf);
        auto* tp = AnaOpacity::EulerTransform(uvw, angleToTurn);

        // Transfer ownership to unique_ptr for automatic cleanup
        thetaTransformed.reset(tp->theta);
        phiTransformed.reset(tp->phi);

        // Clean up intermediate uvw data
        delete uvw->u;
        delete uvw->v;
        delete uvw->w;
        delete uvw;
        // tp struct itself (but NOT its theta/phi vectors which we took ownership of)
        tp->theta = nullptr;  // prevent double-free
        tp->phi = nullptr;
        delete tp;
    } else {
        auto thetaCol = rdf.Take<float>("theta");
        auto phiCol = rdf.Take<float>("phi");
        thetaTransformed = std::make_unique<std::vector<float>>(std::move(*thetaCol));
        phiTransformed = std::make_unique<std::vector<float>>(std::move(*phiCol));
    }

    // ---- Extract array branch components via Define + Take ----
    auto rdfWithArrays = rdf.Define("x_world_0", "static_cast<float>(x_world[0])")
                             .Define("x_world_1", "static_cast<float>(x_world[1])")
                             .Define("x_world_2", "static_cast<float>(x_world[2])")
                             .Define("p0_0", "static_cast<float>(p0[0])")
                             .Define("p0_1", "static_cast<float>(p0[1])")
                             .Define("p0_2", "static_cast<float>(p0[2])");

    auto evtIDCol = rdfWithArrays.Take<int>("EvtID");
    auto detIDCol = rdfWithArrays.Take<int>("DetID");
    auto x0Col = rdfWithArrays.Take<float>("x_world_0");
    auto x1Col = rdfWithArrays.Take<float>("x_world_1");
    auto x2Col = rdfWithArrays.Take<float>("x_world_2");
    auto p0Col0 = rdfWithArrays.Take<float>("p0_0");
    auto p0Col1 = rdfWithArrays.Take<float>("p0_1");
    auto p0Col2 = rdfWithArrays.Take<float>("p0_2");

    const auto nEntries = evtIDCol->size();

    // ---- Build per-EventID groups ----
    std::map<int, std::vector<ScatterEvent>> eventGroups;
    for (size_t i = 0; i < nEntries; ++i) {
        ScatterEvent evt;
        evt.evtID = (*evtIDCol)[i];
        evt.detID = (*detIDCol)[i];
        evt.theta = (*thetaTransformed)[i];
        evt.phi = (*phiTransformed)[i];
        evt.x = (*x0Col)[i];
        evt.y = (*x1Col)[i];
        evt.z = (*x2Col)[i];
        evt.px = (*p0Col0)[i];
        evt.py = (*p0Col1)[i];
        evt.pz = (*p0Col2)[i];
        eventGroups[evt.evtID].push_back(evt);
    }

    // ---- Filter & write CSV ----
    std::ofstream csvFile{cli->get("--output")};
    if (!csvFile.is_open()) {
        throw std::runtime_error{"Failed to open output CSV file."};
    }

    // CSV header
    csvFile << "EvtID,theta_in,phi_in,x_in,y_in,z_in,"
            << "theta_out,phi_out,x_out,y_out,z_out,p_mom\n";

    size_t validCount = 0;
    size_t skippedLessThan2 = 0;
    size_t skippedMoreThan2 = 0;
    size_t skippedMissingDet = 0;

    for (const auto& [evtID, events] : eventGroups) {
        // Collect unique DetIDs in this event group
        std::set<int> detIDs;
        for (const auto& e : events) {
            detIDs.insert(e.detID);
        }

        // Require exactly 2 different DetIDs
        if (detIDs.size() < 2) {
            ++skippedLessThan2;
            continue;
        }
        if (detIDs.size() > 2) {
            std::cerr << "Error: EvtID " << evtID << " has " << detIDs.size()
                      << " different DetIDs (>2), discarding.\n";
            ++skippedMoreThan2;
            continue;
        }

        // Find incoming (DetID=1) and outgoing (DetID=0) events
        const ScatterEvent* inEvt = nullptr;
        const ScatterEvent* outEvt = nullptr;
        for (const auto& e : events) {
            if (e.detID == 1) {
                inEvt = &e;
            } else if (e.detID == 0) {
                outEvt = &e;
            }
        }

        if (!inEvt || !outEvt) {
            std::cerr << "Error: EvtID " << evtID
                      << " missing DetID=0 or DetID=1 (found DetIDs:";
            for (auto d : detIDs) { std::cerr << " " << d; }
            std::cerr << "), discarding.\n";
            ++skippedMissingDet;
            continue;
        }

        // Muon momentum magnitude from incoming p0 (MeV/c → GeV/c)
        const float pMom = std::sqrt(inEvt->px * inEvt->px +
                                     inEvt->py * inEvt->py +
                                     inEvt->pz * inEvt->pz) / 1000.0f;

        // pi - theta gives the angle from the downward vertical direction.
        const float thetaIn = inEvt->theta;
        const float thetaOut = outEvt->theta;

        // Write row: EvtID, theta_in, phi_in, x_in, y_in, z_in,
        //            theta_out, phi_out, x_out, y_out, z_out, p_mom
        csvFile << evtID << ","
                << thetaIn << "," << inEvt->phi << ","
                << inEvt->x << "," << inEvt->y << "," << inEvt->z << ","
                << thetaOut << "," << outEvt->phi << ","
                << outEvt->x << "," << outEvt->y << "," << outEvt->z << ","
                << pMom << "\n";

        ++validCount;
    }

    csvFile.close();

    Mustard::Print("AnaScatter: {} valid events written to CSV.", validCount);
    if (skippedLessThan2 > 0) {
        Mustard::Print("  Skipped {} event groups with < 2 DetIDs.", skippedLessThan2);
    }
    if (skippedMoreThan2 > 0) {
        Mustard::Print("  Skipped {} event groups with > 2 DetIDs.", skippedMoreThan2);
    }
    if (skippedMissingDet > 0) {
        Mustard::Print("  Skipped {} event groups missing DetID=0 or DetID=1.", skippedMissingDet);
    }

    return EXIT_SUCCESS;
}

} // namespace Musae::AnaScatter
