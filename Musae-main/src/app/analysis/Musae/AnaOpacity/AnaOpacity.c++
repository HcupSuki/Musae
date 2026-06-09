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
#include "Musae/AnaOpacity/AnaOpacity.h++"
#include "Musae/AnaOpacity/AnaProcess.h++"

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
#include "TH2D.h"
#include "TPad.h"
#include "TPaletteAxis.h"
#include "TStyle.h"
#include "TRandom3.h"
#include "TChain.h"
#include "TGraph.h"


#include "muc/utility"

#include "gsl/gsl"

#include "G4SystemOfUnits.hh"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <chrono>
#include <filesystem>

namespace Musae::AnaOpacity {

AnaOpacity::AnaOpacity() :
    Subprogram{"AnaOpacity", "Cosmic-ray muon opacity analysis program."} {}

using namespace Mustard::MathConstant;
using namespace std::string_literals;
namespace fs = std::filesystem;
using UnitMap = std::unordered_map<std::string, double>;

auto AnaOpacity::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("-i", "--image-input").help("Image file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--image-tree").help("Image tree name(s), space-separated or comma-separated.").default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"}).required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-j", "--reference-input").help("Reference file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-r", "--reference-tree").help("Reference tree name(s), space-separated or comma-separated.").default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"}).required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-h", "--histogram-model").help("Histogram binning model (nPhi, nTheta, thetaMax, rotatePhi, Histogram palette upper limit, lower limit, filiter num).").required().nargs(7).scan<'g', double>();
    cli->add_argument("-p", "--histogram-palette").help("Histogram palette (integer of ROOT palette enum).").default_value(muc::to_underlying(kViridis)).required().nargs(1).scan<'i', std::underlying_type_t<EColorPalette>>();
    cli->add_argument("-o", "--output").help("Output file path.").nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").default_value("NEW"s).required().nargs(1);
    cli->add_argument("-w", "--weigh-mode").help("Activate weigh analyse mode.(When -w exist turns true)").default_value(false).implicit_value(true);
    cli->add_argument("-c", "--output-csv").help("Output csv file path.").nargs(1);
    cli->add_argument("-n", "--normalization-value").help("Set normalization value(underground / sky * value)").default_value(1.).required().nargs(1).scan<'g', double>();
    cli->add_argument("-u", "--energy-unit").help("Unit of energy.").default_value("GeV"s).required().nargs(1);
    cli->add_argument("-l", "--materiallist-input").help("Input of material list").default_value("../data/Pumas_material/granite_rho2_6.txt"s).required().nargs(1);
    cli->add_argument("-f", "--fluxmodel-input").help("Input of flux model").default_value("../data/Flux_model/Surviaval_to_Ecut_table_pmin30.csv"s).required().nargs(1);
    cli->add_argument("-e", "--euler-transform").help("Euler transform angle(Gamma, Beta, Alpha in degree).").default_value(std::vector<double>{0.0, 0.0, 0.0}).required().nargs(3).scan<'g', double>();
    cli->add_argument("-s", "--search-energy").help("Enable energy search mode").default_value(false).implicit_value(true);
    cli->add_argument("-sm", "--search-energy-mode").help("Define energy search error mode ('Minus' or 'Derivative')").default_value("Minus"s).nargs(1);
    cli->add_argument("-a", "--angular-resolution").help("Input of flux angular resolution histogram").nargs(1);
    cli->add_argument("-b", "--Significance-path").help("Significance root file path.").nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-v", "--Significance-tree").help("Significance tree name(s), space-separated or comma-separated.").default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"}).nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-pre", "--data-prefilter").help("Using gaussian prefilter process survival rate data").default_value(false).implicit_value(true);


    Mustard::Env::BasicEnv env{argc, argv, cli};

    std::unique_ptr<TFile> file;
    std::unique_ptr<TFile> file_ar;
    std::unique_ptr<TApplication> rootApp;
    if (const auto filePath{cli->present("--output")}) {
        file = std::make_unique<TFile>(filePath->c_str(), cli->get("--output-mode").c_str());
        if (not file->IsOpen()) {
            throw std::runtime_error{"Failed to open output file."};
        }
        fs::path original_path(*filePath);
        fs::path parent_dir = original_path.parent_path();
        std::string stem = original_path.stem().string();
    } else {
        rootApp = std::make_unique<TApplication>(argv[0], nullptr, nullptr);
    }

    const auto AngleToTurn{cli->get<std::vector<double>>("--euler-transform")};
    if (AngleToTurn.size() != 3) {
        throw std::invalid_argument("Euler transform angle must have exactly 3 components (Gamma, Beta, Alpha).");
    }
    // const auto AngleToTurn_ref{std::vector<double>{270.,0.}};
    auto imageTreePaths = cli->get<std::vector<std::string>>("--image-tree");
    std::string imageTreeStr = imageTreePaths[0];
    for (size_t i = 1; i < imageTreePaths.size(); ++i) { imageTreeStr += "," + imageTreePaths[i]; }
    auto referenceTreePaths = cli->get<std::vector<std::string>>("--reference-tree");
    std::string referenceTreeStr = referenceTreePaths[0];
    for (size_t i = 1; i < referenceTreePaths.size(); ++i) { referenceTreeStr += "," + referenceTreePaths[i]; }
    std::unique_ptr<TChain> chaini = LoadData(imageTreeStr, cli->get<std::vector<std::string>>("--image-input"));
    std::unique_ptr<TChain> chainr = LoadData(referenceTreeStr, cli->get<std::vector<std::string>>("--reference-input"));
    ROOT::RDataFrame imageData(*chaini);
    ROOT::RDataFrame referenceData{*chainr};

    auto uvw{thetaphi2uvw(imageData)}; // Convert theta and phi to uvw vectors
    auto tp{EulerTransform(uvw, AngleToTurn)};// Apply Euler transform, convert back to theta and phi
    auto uvw_ref{thetaphi2uvw(referenceData)}; // Convert theta and phi to uvw vectors
    auto tp_ref{EulerTransform(uvw_ref, AngleToTurn)};// Apply Euler transform, convert back to theta and phi

    if (const auto filePath_ar{cli->present("--angular-resolution")}) {
        file_ar = std::make_unique<TFile>(filePath_ar->c_str(), "READ");
            if (not file_ar->IsOpen()) {
                throw std::runtime_error{"Failed to open angular resolution file."};
            }
        TH2D* errortable = nullptr;
        file_ar->GetObject("AngleResolutionError", errortable);
        if (!errortable) {
            throw std::runtime_error("Failed to read TH2D.");
        }
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        TRandom3* myRNG = new TRandom3();
        myRNG->SetSeed(seed);
        AError(tp, myRNG, errortable);
        AError(tp_ref, myRNG, errortable);
        file_ar->Close();
        file->cd();
    }

    std::underlying_type_t<EColorPalette> palette{cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette")};
    const auto rawHModel{cli->get<std::vector<double>>("--histogram-model")};
    ROOT::RDF::TH2DModel hModel{"h", "",
                                gsl::narrow<int>(std::lround(rawHModel[0])), -pi, pi,
                                gsl::narrow<int>(std::lround(rawHModel[1])), 0., rawHModel[2]};
    hModel.fName = "ImageHistogram";
    hModel.fTitle = "Underground Muon Counts";
    const auto imageHModel{hModel};
    hModel.fName = "ReferenceHistogram";
    hModel.fTitle = "Open Sky Muon Counts";
    const auto referenceHModel{hModel};
    const auto bonus{cli->get<double>("--normalization-value")};
    const auto minValue{rawHModel[4]}; 
    const auto maxValue{rawHModel[5]}; 
    const auto minentries{rawHModel[6]}; 
    const double energy_unit{unit_value(cli->get<std::string>("--energy-unit"), create_unit_map())};
    const auto materialPath{cli->get("--materiallist-input")};
    const auto fluxmodelPath{cli->get("--fluxmodel-input")};
    const int canvasWidth = 1600, canvasHeight = 1600;
    // const Double_t norm{1.0/30.0/446437.0};
    
    auto imageData_m = imageData.Define("TransPhi",
        [tp](ULong64_t entry) { return (*tp->phi)[entry]; },
        {"rdfentry_"}
    ).Define("TransTheta",
        [tp](ULong64_t entry) { return (*tp->theta)[entry]; },
        {"rdfentry_"}
    );
    

    auto referenceData_m = referenceData.Define("TransPhi",
        [tp_ref](ULong64_t entry) { return (*tp_ref->phi)[entry]; },
        {"rdfentry_"}
    ).Define("TransTheta",
        [tp_ref](ULong64_t entry) { return (*tp_ref->theta)[entry]; },
        {"rdfentry_"}
    );

    // if (const auto output{cli->present("--output")}) {
    //     const auto input{cli->present("--image-input")};
    //     const auto input_ref{cli->present("--reference-input")};
    //     fs::path input_path(*input);
    //     fs::path inputref_path(*input_ref);
    //     fs::path parent_dir = input_path.parent_path();
    //     std::string input_stem = input_path.stem().string();
    //     std::string inputref_stem = inputref_path.stem().string();
    //     std::string new_ext = ".root";
    //     fs::path newinput_path = parent_dir / (input_stem + "_processed" + new_ext);
    //     fs::path newinputref_path = parent_dir / (inputref_stem + "_processed" + new_ext);
    //     imageData_m.Snapshot(cli->get("--image-tree"), newinput_path.c_str());
    //     referenceData_m.Snapshot(cli->get("--reference-tree"), newinputref_path.c_str());
    // }

    TCanvas cImage{"ImageCanvas", "Underground Muon Counts", canvasWidth, canvasHeight};
    ROOT::RDF::Experimental::AddProgressBar(imageData_m);
    ROOT::RDF::RResultPtr<TH2D> hImage;
    if (!cli->get<bool>("--weigh-mode")){
        hImage = imageData_m.Histo2D(imageHModel, "TransPhi", "TransTheta");
        hImage->Sumw2();
    }
    else {
        hImage = imageData_m.Histo2D(imageHModel, "TransPhi", "TransTheta", "w");
    }
    DrawHistogram(cImage, *hImage, palette, rawHModel, file.get());

    TCanvas cReference{"ReferenceCanvas", "Open Sky Muon Counts", canvasWidth, canvasHeight};
    ROOT::RDF::Experimental::AddProgressBar(referenceData_m);
    ROOT::RDF::RResultPtr<TH2D> hReference;
    if (!cli->get<bool>("--weigh-mode")){
        hReference = referenceData_m.Histo2D(referenceHModel, "TransPhi", "TransTheta");
        hReference->Sumw2();
    }
    else {
        hReference = referenceData_m.Histo2D(referenceHModel, "TransPhi", "TransTheta", "w");
    }
    DrawHistogram(cReference, *hReference, palette, rawHModel, file.get());

    TCanvas cOpacity{"SurvivalRateCanvas", "Muon Survival Rate", canvasWidth, canvasHeight};
    const std::unique_ptr<TH2D> hOpacity{dynamic_cast<TH2D*>(hImage->Clone())};
    hOpacity->SetNameTitle("SurvivalRateHistogram", "Muon Survival Rate");
    // hOpacity->Divide(&*hReference);
    hOpacity->Divide(&*hImage, &*hReference, 1.0, 1.0);
    for (int i = 1; i <= hOpacity->GetNbinsX(); ++i) {
        for (int j = 1; j <= hOpacity->GetNbinsY(); ++j) {
            if (hImage->GetBinContent(i, j) < minentries || 
                hReference->GetBinContent(i, j) < minentries) {
                hOpacity->SetBinContent(i, j, 0);
                hOpacity->SetBinError(i, j, 0);
            }
        }
    }
    hOpacity->Scale(bonus);
    if (cli->get<bool>("--data-prefilter")){
        auto hOpacity_pre{GaussianFilter(&*hOpacity, 3, 1)};
        hOpacity->Reset();
        hOpacity->Add(&*hOpacity_pre);
    }
    DrawHistogram(cOpacity, *hOpacity, minValue, maxValue, palette, rawHModel, file.get());
    
    if (const std::vector<std::string> SignificancefilePath{cli->get<std::vector<std::string>>("--Significance-path")};
        !SignificancefilePath.empty()) 
    {
        auto significanceTreePaths = cli->get<std::vector<std::string>>("--Significance-tree");
        std::string significanceTreeStr = significanceTreePaths[0];
        for (size_t i = 1; i < significanceTreePaths.size(); ++i) { significanceTreeStr += "," + significanceTreePaths[i]; }
        std::unique_ptr<TChain> chaing = LoadData(significanceTreeStr, SignificancefilePath);
        ROOT::RDataFrame SignificanceData(*chaing);
        hModel.fName = "SignificanceHistogram";
        hModel.fTitle = "Significance";
        const auto SignificanceHModel{hModel};

        auto uvws{thetaphi2uvw(SignificanceData)}; // Convert theta and phi to uvw vectors
        auto tps{EulerTransform(uvws, AngleToTurn)};// Apply Euler transform, convert back to theta and phi

        if (const auto filePath_ar{cli->present("--angular-resolution")}) {
        file_ar = std::make_unique<TFile>(filePath_ar->c_str(), "READ");
            if (not file_ar->IsOpen()) {
                throw std::runtime_error{"Failed to open angular resolution file."};
            }
        TH2D* errortable = nullptr;
        file_ar->GetObject("AngleResolutionError", errortable);
        if (!errortable) {
            throw std::runtime_error("Failed to read TH2D.");
        }
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        TRandom3* myRNG = new TRandom3();
        myRNG->SetSeed(seed);
        AError(tps, myRNG, errortable);
        file_ar->Close();
        file->cd();
        }

        auto SignificanceData_m = SignificanceData.Define("TransPhi",
            [tps](ULong64_t entry) { return (*tps->phi)[entry]; },
            {"rdfentry_"}
        ).Define("TransTheta",
            [tps](ULong64_t entry) { return (*tps->theta)[entry]; },
            {"rdfentry_"}
        );

        ROOT::RDF::RResultPtr<TH2D> hSignificance;
        if (!cli->get<bool>("--weigh-mode")){
            hSignificance = SignificanceData_m.Histo2D(SignificanceHModel, "TransPhi", "TransTheta");
            hSignificance->Sumw2();
        }
        else {
            hSignificance = SignificanceData_m.Histo2D(SignificanceHModel, "TransPhi", "TransTheta", "w");
        }

        TCanvas cOpacity_sig{"SurvivalRateCanvas_sig", "Muon Survival Rate", canvasWidth, canvasHeight};
        const std::unique_ptr<TH2D> hOpacity_sig{dynamic_cast<TH2D*>(hSignificance->Clone())};
        hOpacity_sig->SetNameTitle("SurvivalRateHistogram_sig", "Muon Survival Rate");
        // hOpacity->Divide(&*hReference);
        hOpacity_sig->Divide(&*hSignificance, &*hReference, 1.0, 1.0);
        hOpacity_sig->Scale(bonus);
        DrawHistogram(cOpacity_sig, *hOpacity_sig, minValue, maxValue, palette, rawHModel, file.get());

        TCanvas cSignificance{"SignificanceCanvas", "Statistical Significance", canvasWidth, canvasHeight};
        const std::unique_ptr<TH2D> hSignificanceResult{dynamic_cast<TH2D*>(hImage->Clone())};
        hSignificanceResult->SetNameTitle("SignificanceHistogram", "Statistical Significance");
        hSignificanceResult->Reset();
        Significance(*hOpacity, *hOpacity_sig, *hSignificanceResult);
        DrawHistogram(cSignificance, *hSignificanceResult, palette, rawHModel, file.get());
    }


    if(cli->get<bool>("--search-energy"))
    {
    auto hResult{SerachEandR(*hOpacity, materialPath, fluxmodelPath, energy_unit, cli->get<std::string>("--search-energy-mode"))};
    TCanvas cRange{"OpacityCanvas", "Opacity", canvasWidth, canvasHeight};
    // DrawHistogram_rec(cRange, *hResult->first, palette, file.get());
    // hResult->first->Scale(1/10000.);
    // hResult->second->Scale(1/10000.);
    DrawHistogram(cRange, *hResult->first, palette, rawHModel, file.get());
    TCanvas cEmin{"EminCanvas", "E_{min}", canvasWidth, canvasHeight};
    // DrawHistogram_rec(cEmin, *hResult->second, palette, file.get());
    DrawHistogram(cEmin, *hResult->second, palette, rawHModel, file.get());

    std::unique_ptr<TH2D> hMean{dynamic_cast<TH2D*>(hResult->first->Clone())};
    hMean->Reset();
    hMean->SetNameTitle("MeanFilterHistogram", "Mean Filter");
    MeanFilter(hResult->first, *hMean, 3);
    TCanvas cMeanFilter{"MeanFilterCanvas", "MeanFilter", canvasWidth, canvasHeight};
    // DrawHistogram_rec(cMeanFilter, *hMean, palette, file.get());
    DrawHistogram(cMeanFilter, *hMean, palette, rawHModel, file.get());

    auto hGaussian{GaussianFilter(hResult->first, 3, 1)};
    TCanvas cGaussianFilter{"GaussianFilterCanvas", "GaussianFilter", canvasWidth, canvasHeight};
    // DrawHistogram_rec(cGaussianFilter, *hGaussian, palette, file.get());
    DrawHistogram(cGaussianFilter, *hGaussian, palette, rawHModel, file.get());
    
    auto hMedian{MedianFilter(hResult->first, 3)};
    TCanvas cMedianFilter{"MedianFilterCanvas", "MedianFilter", canvasWidth, canvasHeight};
    // DrawHistogram_rec(cMedianFilter, *hMedian, palette, file.get());
    DrawHistogram(cMedianFilter, *hMedian, palette, rawHModel, file.get());

    if (const auto CsvfilePath{cli->present("--output-csv")}) 
    {
        Csvout(*hResult->first, CsvfilePath->c_str());
        fs::path csv_path(*CsvfilePath);
        fs::path parent_dir = csv_path.parent_path();
        std::string input_stem = csv_path.stem().string();
        std::string new_ext = ".csv";
        fs::path meanPath = parent_dir / (input_stem + "_MeanFilter" + new_ext);
        fs::path gaussianPath = parent_dir / (input_stem + "_GaussianFilter" + new_ext);
        fs::path medianPath = parent_dir / (input_stem + "_MedianFilter" + new_ext);
        Csvout(*hMean, meanPath.c_str());
        Csvout(*hGaussian, gaussianPath.c_str());
        Csvout(*hMedian, medianPath.c_str());
    }
    }
    
    if (rootApp) {
        Mustard::Print("Press enter twice to exit...");
        std::getchar();
        Mustard::Print("Press enter again to exit...");
        std::getchar();
    }

    
    return EXIT_SUCCESS;
}
} // namespace Musae::AnaOpacity