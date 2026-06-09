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
#include "Mustard/Utility/LiteralUnit.h++"

#include "G4SystemOfUnits.hh"

#include "ROOT/RDFHelpers.hxx"
#include "ROOT/RDataFrame.hxx"
#include "TApplication.h"
#include "TCanvas.h"
#include "TFile.h"
#include "TGraphPolargram.h"
#include "TH2D.h"
#include "TPad.h"
#include "TPaletteAxis.h"
#include "TStyle.h"
#include "TRandom3.h"
#include "TChain.h"
#include "TFrame.h"

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

namespace Musae::AnaOpacity {
    
using namespace std;
using UnitMap = std::unordered_map<std::string, double>;

struct uvw {
    std::vector<float>* u;
    std::vector<float>* v;
    std::vector<float>* w;
};

struct tp {
    std::vector<float>* theta;
    std::vector<float>* phi;
};

struct BinData {
    std::vector<float> energies;
    std::vector<float> weights;
    float total_weight = 0.0;
};



void DrawHistogram(TCanvas& c, TH2D& h, double minValue, double maxValue, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file);
void DrawHistogram(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file);
void DrawHistogram_rec(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, TFile* file);
void Csvout(TH2D& hOpacity, const char* CsvfilePath);
vector<string> split(const string& s);
map<string, vector<double>> MaterialLoad(const string& filename, const vector<string>& selectedColumns);
std::pair<double, double>* SearchRange(const string& filename, const double Ecut, const double error);
inline UnitMap create_unit_map() {return{{"MeV",  CLHEP::MeV},{"GeV",  CLHEP::GeV},{"TeV",  CLHEP::TeV},{"eV",  CLHEP::eV}};}
double unit_value(const std::string& unit_str, UnitMap unitmap);
std::pair<double, double>* lqpoint(long long int index, double Ecut_value, const map<string, vector<double>>& data);
std::pair<TH2D*, TH2D*>* SerachEandR(const TH2D& ref_h, const std::string& materialname, const std::string& fluxname, double unit, std::string search_mode);
// std::pair<TH2D*, TH2D*>* SerachEcut(ROOT::RDF::RInterface<ROOT::Detail::RDF::RLoopManager, void> rdf_m, const TH2D& ref_h, const std::string& materialname, int min_entries, double unit) ;
std::pair<double, double>* Esearch(const double theta, const double survival_rate, const double error, std::vector<std::vector<double>> &data);
uvw* thetaphi2uvw(ROOT::RDataFrame& rdf);
tp* EulerTransform(uvw* Ddata, std::vector<double> AngleToTurn);
void AError(tp* Adata, TRandom3* myRNG, TH2D* errortable);
// ROOT::RDF::RInterface<ROOT::Detail::RDF::RLoopManager, void> Inclination(ROOT::RDataFrame& rdf, std::vector<double> AngleToTurn);
void Significance(TH2D& h_original, TH2D& h_reference, TH2D& h_out);
void MeanFilter(TH2D* h_original, TH2D& h_out, int filter_size);
TH2D* GaussianFilter(TH2D* h_original, const int filter_size,const double sigma);
TH2D* MedianFilter(TH2D* h_original, const int filter_size);
std::vector<std::vector<double>> GenerateGaussianKernel(int size, double sigma);
std::unique_ptr<TChain> LoadData(std::string treePaths, std::vector<std::string> inputFiles);

} // namespace Musae::AnaOpacity
