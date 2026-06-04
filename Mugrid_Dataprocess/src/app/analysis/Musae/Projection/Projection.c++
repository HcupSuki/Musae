#include "Musae/Projection/Projection.h++"
#include "Musae/Projection/ProjectProcess.h++"

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
#include "TTreeReader.h"
#include "TTreeReaderValue.h"
#include "TRandom3.h"
#include "TChain.h"


#include "muc/utility"
#include "muc/array"

#include "gsl/gsl"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Musae::Projection {

Projection::Projection() :
    Subprogram{"Projection", "Cosmic-ray muon opacity Projection program."} {}

using namespace Mustard::MathConstant;
using namespace std::string_literals;

auto Projection::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("-i", "--image-input").help("Image file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--image-tree").help("Image tree name(s), space-separated or comma-separated.").default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"}).required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-j", "--reference-input").help("Reference file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-r", "--reference-tree").help("Reference tree name(s), space-separated or comma-separated.").default_value(std::vector<std::string>{"G4Run0/CRMuSimEvent"}).required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-h", "--histogram-model").help("Histogram binning model (nx, ny, xMin, xMax, yMin, yMax, residualMax(set 99 to disable), residualMin(set 99 to disable), cutValue).").required().nargs(9).scan<'g', double>();
    cli->add_argument("-p", "--histogram-palette").help("Histogram palette (integer of ROOT palette enum).").default_value(muc::to_underlying(kViridis)).required().nargs(1).scan<'i', std::underlying_type_t<EColorPalette>>();
    cli->add_argument("-o", "--output").help("Output file path.").nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").default_value("NEW"s).required().nargs(1);
    cli->add_argument("-w", "--weigh-mode").help("Activate weigh analyse mode.(When -w exist turns true)").default_value(false).implicit_value(true);
    cli->add_argument("-a", "--backproject-length").help("Backproject length(m).").default_value(0.).nargs(1).scan<'g', float>();
    cli->add_argument("-u", "--histogram-upperlimit").help("Histogram palette upper limit.").default_value(1.).required().nargs(1).scan<'g', double>();
    cli->add_argument("-l", "--histogram-lowerlimit").help("Histogram palette lower limit.").default_value(0.).required().nargs(1).scan<'g', double>();
    cli->add_argument("-n", "--normalization-value").help("Set normalization value(underground / sky * value)").default_value(1.).required().nargs(1).scan<'g', double>();
    cli->add_argument("-k", "--angular-resolution").help("Input of flux angular resolution histogram").nargs(1);
    cli->add_argument("-e", "--euler-transform").help("Euler transform angle(Gamma, Beta, Alpha in degree).").default_value(std::vector<double>{0.0, 0.0, 0.0}).required().nargs(3).scan<'g', double>();
    Mustard::Env::BasicEnv env{argc, argv, cli};

    std::unique_ptr<TFile> file;
    std::unique_ptr<TFile> file_ar;
    std::unique_ptr<TApplication> rootApp;
    if (const auto filePath{cli->present("--output")}) {
        file = std::make_unique<TFile>(filePath->c_str(), cli->get("--output-mode").c_str());
        if (not file->IsOpen()) {
            throw std::runtime_error{"Failed to open output file."};
        }
    } else {
        rootApp = std::make_unique<TApplication>(argv[0], nullptr, nullptr);
    }

    const auto AngleToTurn{cli->get<std::vector<double>>("--euler-transform")};
    if (AngleToTurn.size() != 3) {
        throw std::invalid_argument("Euler transform angle must have exactly 3 components (Gamma, Beta, Alpha).");
    }
    auto imageTreePaths = cli->get<std::vector<std::string>>("--image-tree");
    std::string imageTreeStr = imageTreePaths[0];
    for (size_t i = 1; i < imageTreePaths.size(); ++i) { imageTreeStr += "," + imageTreePaths[i]; }
    auto referenceTreePaths = cli->get<std::vector<std::string>>("--reference-tree");
    std::string referenceTreeStr = referenceTreePaths[0];
    for (size_t i = 1; i < referenceTreePaths.size(); ++i) { referenceTreeStr += "," + referenceTreePaths[i]; }
    std::unique_ptr<TChain> chaini = LoadData(imageTreeStr, cli->get<std::vector<std::string>>("--image-input"));
    std::unique_ptr<TChain> chainr = LoadData(referenceTreeStr, cli->get<std::vector<std::string>>("--reference-input"));
    const auto rawHModel{cli->get<std::vector<double>>("--histogram-model")};
    const auto bonus{cli->get<double>("--normalization-value")};

    auto chaini_transformed = ApplyEulerToChain(
        chaini.get(),
        "temp_image_tree",
        AngleToTurn, cli->get<bool>("--weigh-mode")
    );

    auto chainr_transformed = ApplyEulerToChain(
        chainr.get(),
        "temp_reference_tree",
        AngleToTurn, cli->get<bool>("--weigh-mode")
    );

        
    const auto length{cli->get<float>("--backproject-length")};
    TH2D* hProjection = new TH2D("hProjection", "Backprojection",
        gsl::narrow<int>(std::lround(rawHModel[0])), rawHModel[2], rawHModel[3],
        gsl::narrow<int>(std::lround(rawHModel[1])), rawHModel[4], rawHModel[5]);
    hProjection->GetXaxis()->SetTitle("x (mm)");
    hProjection->GetYaxis()->SetTitle("y (mm)");
    hProjection->SetStats(false);
    auto lx{std::abs(rawHModel[3]-rawHModel[2])};
    auto ly{std::abs(rawHModel[5]-rawHModel[4])};
    const auto rawHeight{1600 * ly / lx};
    // const float rightMargin = 0.15f;  // Adjust based on actual colorbar width
    const auto height = static_cast<Int_t>(rawHeight);
    // const auto width = static_cast<Int_t>(800 / (1.0f - rightMargin));
    const auto width = 1600;
    if (std::abs(rawHeight - height) > 1e-9) {
        std::cerr << "Warning: canvas height " << rawHeight
                << " truncated to " << height << std::endl;
    }
    TCanvas cImage{"BackprojectionCanvas", "Backprojection", width, height};

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


        BackProjectA(myRNG, errortable, *hProjection,
            chaini_transformed.get(), length, cli->get<bool>("--weigh-mode"));


    }
    else{
        BackProject(*hProjection,
            chaini_transformed.get(), length, cli->get<bool>("--weigh-mode"));
    }
    Draw(cImage, *hProjection, cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette"), file.get(), "Muon Counts");

    TH2D* hProjection_r = new TH2D("hProjection_opensky", "Backprojection_opensky",
        gsl::narrow<int>(std::lround(rawHModel[0])), rawHModel[2], rawHModel[3],
        gsl::narrow<int>(std::lround(rawHModel[1])), rawHModel[4], rawHModel[5]);
    hProjection_r->GetXaxis()->SetTitle("x (mm)");
    hProjection_r->GetYaxis()->SetTitle("y (mm)");
    hProjection_r->SetStats(false);

    TCanvas cImage_r{"BackprojectionCanvas_opensky", "Backprojection_opensky", width, height};
    
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
        BackProjectA(myRNG, errortable, *hProjection_r,
            chainr_transformed.get(), length, cli->get<bool>("--weigh-mode"));
    }
    else{
        BackProject(*hProjection_r,
            chainr_transformed.get(), length, cli->get<bool>("--weigh-mode"));
    }
    hProjection_r->Scale(bonus);
    Draw(cImage_r, *hProjection_r, cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette"), file.get(), "Muon Counts");
    
    TCanvas cImage_s{"BackprojectionCanvas_Survival_Rate", "Backprojection_Survival_Rate", width, height};
    const std::unique_ptr<TH2D> hOpacity{dynamic_cast<TH2D*>(hProjection->Clone())};
    hOpacity->SetNameTitle("Backprojection Survival Rate", "Backprojection Survival Rate");
    hOpacity->Divide(&*hProjection_r);
    Draw(cImage_s, *hOpacity, cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette"), file.get(), "Survival Rate");

    TH1D* pullDist = new TH1D("pullDist", "Standardized Residual Distribution", 100, -7, 7);
    TCanvas cImage_p{"Standardized_Residual_Distribution", "Standardized Residual Distribution", width, height};
    TCanvas cImage_p1{"Standardized_Residual_1D", "Standardized Residual 1D Distribution", width, height};

    const std::unique_ptr<TH2D> hPull{dynamic_cast<TH2D*>(hProjection->Clone())};
    hPull->Reset();
    hPull->SetNameTitle("Standardized Residual Distribution", "Standardized Residual Distribution");
    hPull->GetXaxis()->SetTitle("x (mm)");
    hPull->GetYaxis()->SetTitle("y (mm)");
    hPull->SetStats(false);
    PullDistribution(*hProjection, *hProjection_r, *hPull, *pullDist, rawHModel[6], rawHModel[7], static_cast<Int_t>(rawHModel[8]));
    Draw(cImage_p, *hPull, cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette"), file.get(), "Residual");
    cImage_p.SaveAs("Standardized_Residual_Distribution.png");
    Draw_1(cImage_p1, *pullDist, file.get());

    
    if (rootApp) {
        Mustard::Print("Press enter twice to exit...");
        std::getchar();
        Mustard::Print("Press enter again to exit...");
        std::getchar();
    }
    
    return EXIT_SUCCESS;
}

} // namespace Musae::Projection