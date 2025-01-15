#include "Musae/AnaOpacity/AnaOpacity.h++"

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

#include "muc/utility"

#include "gsl/gsl"

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <numbers>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace Musae::AnaOpacity {

AnaOpacity::AnaOpacity() :
    Subprogram{"AnaOpacity", "Cosmic-ray muon opacity analysis program."} {}

using namespace Mustard::MathConstant;
using namespace std::string_literals;

auto AnaOpacity::Main(int argc, char* argv[]) const -> int {
    Mustard::Env::CLI::BasicCLI<> cli;
    cli->add_argument("-i", "--image-input").help("Image file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-t", "--image-tree").help("Image tree name.").default_value("CRMuEvent"s).required().nargs(1);
    cli->add_argument("-j", "--reference-input").help("Reference file path(s).").required().nargs(argparse::nargs_pattern::at_least_one);
    cli->add_argument("-r", "--reference-tree").help("Reference tree name.").default_value("CRMuEvent"s).required().nargs(1);
    cli->add_argument("-h", "--histogram-model").help("Histogram binning model (nPhi, nTheta, thetaMax).").required().nargs(3).scan<'g', double>();
    cli->add_argument("-p", "--histogram-palette").help("Histogram palette (integer of ROOT palette enum).").default_value(muc::to_underlying(kViridis)).required().nargs(1).scan<'i', std::underlying_type_t<EColorPalette>>();
    cli->add_argument("-o", "--output").help("Output file path.").nargs(1);
    cli->add_argument("-m", "--output-mode").help("Output file creation mode.").default_value("NEW"s).required().nargs(1);
    Mustard::Env::BasicEnv env{argc, argv, cli};

    std::unique_ptr<TFile> file;
    std::unique_ptr<TApplication> rootApp;
    if (const auto filePath{cli->present("--output")}) {
        file = std::make_unique<TFile>(filePath->c_str(), cli->get("--output-mode").c_str());
        if (not file->IsOpen()) {
            return EXIT_FAILURE;
        }
    } else {
        rootApp = std::make_unique<TApplication>(argv[0], nullptr, nullptr);
    }

    ROOT::RDataFrame imageData{cli->get("--image-tree"), cli->get<std::vector<std::string>>("--image-input")};
    ROOT::RDataFrame referenceData{cli->get("--reference-tree"), cli->get<std::vector<std::string>>("--reference-input")};

    const auto rawHModel{cli->get<std::vector<double>>("--histogram-model")};
    ROOT::RDF::TH2DModel hModel{"h", "",
                                gsl::narrow<int>(std::lround(rawHModel[0])), -pi, pi,
                                gsl::narrow<int>(std::lround(rawHModel[1])), 0., rawHModel[2]};
    hModel.fName = "ImageHistogram";
    hModel.fTitle = "Image";
    const auto imageHModel{hModel};
    hModel.fName = "ReferenceHistogram";
    hModel.fTitle = "Reference";
    const auto referenceHModel{hModel};

    const auto DrawHistogram{
        [&](TCanvas& c, TH2D& h) {
            gStyle->SetNumberContours(99);
            TStyle style;
            style.SetPalette(cli->get<std::underlying_type_t<EColorPalette>>("--histogram-palette"));
            style.SetNumberContours(99);
            style.Draw();

            c.SetTheta(90);
            c.SetPhi(180); // 180 to fit the axis!

            h.SetLineColorAlpha(kBlack, 0);
            h.GetXaxis()->SetTitle("#phi (rad)");
            h.GetYaxis()->SetTitle("#theta (rad)");
            h.SetStats(false);
            h.SetDrawOption("lego2 polz");
            h.Draw("lego2 polz");

            TPad polarPad{"p", "p", 0, 0, 1, 1};
            polarPad.SetFillStyle(4444); // transparent
            polarPad.Draw();
            polarPad.cd();
            TGraphPolargram polarAxis{"g", 0, rawHModel[2], 0, 2 * pi};
            polarAxis.SetNdivPolar(8);
            polarAxis.SetNdivRadial(4);
            polarAxis.Draw("O");

            c.Update();
            const auto palette{dynamic_cast<TPaletteAxis*>(h.GetListOfFunctions()->FindObject("palette"))};
            palette->SetY1NDC(0.55);
            palette->SetY2NDC(0.9);
            c.Modified();
            c.Update();

            if (file) {
                h.Write();
                c.Write();
            }
        }};

    TCanvas cImage{"ImageCanvas", "Image", 600, 600};
    ROOT::RDF::Experimental::AddProgressBar(imageData);
    auto hImage{imageData.Histo2D(imageHModel, "phi", "theta")};
    hImage->Scale(1 / hImage->Integral(), "width");
    DrawHistogram(cImage, *hImage);

    TCanvas cReference{"ReferenceCanvas", "Reference", 600, 600};
    ROOT::RDF::Experimental::AddProgressBar(referenceData);
    auto hReference{referenceData.Histo2D(referenceHModel, "phi", "theta")};
    hReference->Scale(1 / hReference->Integral(), "width");
    DrawHistogram(cReference, *hReference);

    TCanvas cOpacity{"OpacityCanvas", "Opacity", 600, 600};
    const std::unique_ptr<TH2D> hOpacity{dynamic_cast<TH2D*>(hImage->Clone())};
    hOpacity->SetNameTitle("OpacityHistogram", "Opacity");
    hOpacity->Divide(&*hReference);
    DrawHistogram(cOpacity, *hOpacity);

    if (rootApp) {
        Mustard::Print("Press enter twice to exit...");
        std::getchar();
        Mustard::Print("Press enter again to exit...");
        std::getchar();
    }

    return EXIT_SUCCESS;
}

} // namespace Musae::AnaOpacity
