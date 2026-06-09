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
#pragma once

#include "Mustard/Env/BasicEnv.h++"
#include "Mustard/Env/CLI/BasicCLI.h++"
#include "Mustard/Utility/MathConstant.h++"
#include "Mustard/Utility/Print.h++"

#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDataFrame.hxx"
#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TGraphPolargram.h"
#include "TF1.h"
#include "TH2D.h"
#include "TPad.h"
#include "TPaletteAxis.h"
#include "TStyle.h"
#include "TAxis.h"
#include "TRandom3.h"
#include "TChain.h"


#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <string>
#include <type_traits>
#include <utility>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <iomanip>

namespace Musae::Projection {
    
using namespace std;
struct uvw {
    std::vector<float>* u;
    std::vector<float>* v;
    std::vector<float>* w;
};

struct tp {
    std::vector<float>* theta;
    std::vector<float>* phi;
};


struct ChainWithTmpFile {
    std::string filepath;
    void operator()(TChain* chain) const {
        delete chain;
        std::remove(filepath.c_str()); // delete file after TChain destruction
    }
};

using ManagedChain = std::unique_ptr<TChain, ChainWithTmpFile>;

void DrawHistogram(TCanvas& c, TH2D& h, TH2D& hTransformed, double minValue, double maxValue, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file);
void DrawHistogram(TCanvas& c, TH2D& h, TH2D& hTransformed, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file);
void Draw(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, TFile* file, const std::string& title = "");
void Draw_1(TCanvas& c, TH1D& h, TFile* file);
void BackProject(TH2D& h, TChain* chain, float length, bool isW = false);
void PullDistribution(TH2D& ho, TH2D& hr, TH2D& hout, TH1D& h1, double maxValue=99, double minValue=99, int cut=0);
void BackProjectA(TRandom3* myRNG, TH2D* errortable, TH2D& h, TChain* chain, float length, bool isW = false);
uvw* thetaphi2uvw(ROOT::RDataFrame& rdf);
tp* EulerTransform(uvw* Ddata, std::vector<double> AngleToTurn);
ManagedChain ApplyEulerToChain(TChain* chain, const std::string& treeName, std::vector<double> AngleToTurn, bool isW = false);

std::unique_ptr<TChain> LoadData(std::string treePaths, std::vector<std::string> inputFiles);

} // namespace Musae::Projection
