#include "Musae/Projection/ProjectProcess.h++"

#include "TAxis.h"
#include "TLatex.h"



namespace Musae::Projection {

    using namespace Mustard::MathConstant;
    using namespace std::string_literals;
    using namespace std;

    

    void DrawHistogram(TCanvas& c, TH2D& h, TH2D& hTransformed, double minValue, double maxValue, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file)
    {
        gStyle->SetNumberContours(256);

        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(256);
        style.Draw();

        
        // Set fixed color range
        hTransformed.SetMinimum(minValue);
        hTransformed.SetMaximum(maxValue);
    
        for (int i = 1; i <= rawHModel[0]; ++i) {
            for (int j = 1; j <= rawHModel[1]; ++j) {
                // Get original bin content
                double content = h.GetBinContent(i, j);
                double error = h.GetBinError(i, j);
                
                // Compute new bin position (index reversal)
                // int newI = rawHModel[0] - i + 1;
                int newJ = rawHModel[1] - j + 1;
                int newI = i;
                // int newJ = j;
                
                // Fill into new histogram
                hTransformed.SetBinContent(newI, newJ, content);
                hTransformed.SetBinError(newI, newJ, error);
            }
        }

        hTransformed.GetXaxis()->SetRangeUser(rawHModel[2], rawHModel[3]);
        hTransformed.GetYaxis()->SetRangeUser(rawHModel[4], rawHModel[5]);
        hTransformed.SetLineColorAlpha(kBlack, 0);
        hTransformed.GetXaxis()->SetTitle("#phi (rad)");
        hTransformed.GetYaxis()->SetTitle("#theta (rad)");
        hTransformed.SetStats(false);
        hTransformed.Draw("colz RX");

        c.Update();
        const auto palette{dynamic_cast<TPaletteAxis*>(hTransformed.GetListOfFunctions()->FindObject("palette"))};
        palette->SetY1NDC(0.1);
        palette->SetY2NDC(0.9);
        c.Modified();
        c.Update();

        if (file) {
            h.Write();
            hTransformed.Write();
            c.Write();
        }
    }

    void DrawHistogram(TCanvas& c, TH2D& h, TH2D& hTransformed, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file)
    {

        gStyle->SetNumberContours(256);

        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(256);
        style.Draw();


        for (int i = 1; i <= rawHModel[0]; ++i) {
            for (int j = 1; j <= rawHModel[1]; ++j) {
                // Get original bin content
                double content = h.GetBinContent(i, j);
                double error = h.GetBinError(i, j);
                
                // Compute new bin position (index reversal)
                // int newI = rawHModel[0] - i + 1;
                int newJ = rawHModel[1] - j + 1;

                int newI = i;
                // int newJ = j;
                
                // Fill into new histogram
                hTransformed.SetBinContent(newI, newJ, content);
                hTransformed.SetBinError(newI, newJ, error);
            }
        }

        hTransformed.GetXaxis()->SetRangeUser(rawHModel[2], rawHModel[3]);
        hTransformed.GetYaxis()->SetRangeUser(rawHModel[4], rawHModel[5]);
        hTransformed.SetLineColorAlpha(kBlack, 0);
        hTransformed.GetXaxis()->SetTitle("#phi (rad)");
        hTransformed.GetYaxis()->SetTitle("#theta (rad)");
        hTransformed.SetStats(false);
        hTransformed.Draw("colz RX");

        c.Update();
        const auto palette{dynamic_cast<TPaletteAxis*>(hTransformed.GetListOfFunctions()->FindObject("palette"))};
        palette->SetY1NDC(0.1);
        palette->SetY2NDC(0.9);
        c.Modified();
        c.Update();

        if (file) {
            h.Write();
            hTransformed.Write();
            c.Write();
            
        }
    }

    void Draw(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, TFile* file, const std::string& title)
    {
        gStyle->SetNumberContours(256);

        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(256);
        style.Draw();

        c.Update();

        c.cd();
        // Get x and y axis objects
        TAxis* xaxis = h.GetXaxis();
        TAxis* yaxis = h.GetYaxis();
        // Set title font size
        xaxis->SetTitleSize(0.035);    // x-axis title font size
        yaxis->SetTitleSize(0.035);    // y-axis title font size
        // Set title offset
        xaxis->SetTitleOffset(1);   // x-axis title offset
        yaxis->SetTitleOffset(1.5);   // y-axis title offset
        // // Optional: set label font size
        xaxis->SetLabelSize(0.025);    // x-axis label font size
        yaxis->SetLabelSize(0.025);    // y-axis label font size

        h.Draw("colz");
        c.Update();

        const auto palette{dynamic_cast<TPaletteAxis*>(h.GetListOfFunctions()->FindObject("palette"))};
        if (!palette) {
            std::cerr << "Error: Palette axis not found!" << std::endl;
            return;
        }
        palette->SetY1NDC(0.1);
        palette->SetY2NDC(0.9);
        palette->SetLabelSize(0.025);
        c.Modified();
        c.Update();

        TLatex* colorbar_title = new TLatex();
        colorbar_title->SetNDC();
        colorbar_title->SetTextSize(0.025);
        // colorbar_title->SetTextAngle(90); // vertical text
        colorbar_title->DrawLatex(0.88, 0.935, title.c_str()); // Add title to the right of colorbar
        // colorbar_title->SetTextAlign(21); // center horizontally + align top
        c.Modified();
        c.Update();

        if (file) {
            file->cd();
            h.Write();
            c.Write();
        }
    }
    void Draw_1(TCanvas& c, TH1D& h, TFile* file)
    {
        c.cd();
        h.Draw("HIST");
        c.Update();

        if (file) {
            file->cd();
            h.Write();
            c.Write();
        }
    }

    void BackProject(TH2D& h, TChain* chain, float length, bool isW)
    {
        float x0_array[3], theta, phi, step;
        chain->SetBranchAddress("x0", &x0_array);// unit: mm, range: +/-150mm (300mm detector)
        chain->SetBranchAddress("theta", &theta);
        chain->SetBranchAddress("phi", &phi);

        if (isW) {
            float w;
            chain->SetBranchAddress("w", &w);

            Long64_t nEntries = chain->GetEntries();
            for (Long64_t i = 0; i < nEntries; i++) {
                chain->GetEntry(i);
                if (cos(theta) <= 0) {
                    continue; // Skip invalid theta values
                }
                step = length * 1000 / cos(theta);
                h.Fill(
                    x0_array[0] + step * sin(theta) * cos(phi),
                    x0_array[1] + step * sin(theta) * sin(phi),
                    w
                );
            }
            h.Sumw2(kFALSE);  // Disable default weighted error calculation to avoid double counting

            for (int ix = 1; ix <= h.GetNbinsX(); ++ix) {
                for (int iy = 1; iy <= h.GetNbinsY(); ++iy) {
                    int bin = h.GetBin(ix, iy);
                    double content = h.GetBinContent(bin);
                    h.SetBinError(bin, std::sqrt(content));
                }
            }
        }
        else{
        Long64_t nEntries = chain->GetEntries();
        for (Long64_t i = 0; i < nEntries; i++) {
            chain->GetEntry(i);
            if (cos(theta) <= 0) {
                    continue; // Skip invalid theta values
                }
            step = length * 1000 / cos(theta);
            h.Fill(
                x0_array[0] + step * sin(theta) * cos(phi),
                x0_array[1] + step * sin(theta) * sin(phi)
            );
        }
        }
    }
    void PullDistribution(TH2D& ho, TH2D& hr, TH2D& hout, TH1D& h1, double maxValue, double minValue, int cut)
    {

        for (int i = 1; i <= ho.GetNbinsX(); i++) {
            for (int j = 1; j <= ho.GetNbinsY(); j++) {
                // Get content and error for each bin
                auto content1 = ho.GetBinContent(i, j);
                auto content2 = hr.GetBinContent(i, j);
                if (cut != 0 && (content1 <= cut || content2 <= cut)) {
                    continue;
                }
                auto error1 = ho.GetBinError(i, j);
                auto error2 = hr.GetBinError(i, j);
                auto denominator = TMath::Sqrt(error1*error1 + error2*error2);
                if (denominator > 0) {
                    auto pull = (content1 - content2) / denominator;
                    if (maxValue != 99 && pull > maxValue) {
                        pull = maxValue;
                    }
                    if (minValue != 99 && pull < minValue) {
                        pull = minValue;
                    }
                    hout.SetBinContent(i, j, pull);
                    h1.Fill(pull); // Fill pull value distribution
                } else {
                    auto pull = (content1 == content2) ? 0 : ((content1 > content2) ? 999 : -999);
                    hout.SetBinContent(i, j, pull);
                    h1.Fill(pull); // Fill pull value distribution
                }
        }}

    }

    std::unique_ptr<TChain> LoadData(std::string treePaths, std::vector<std::string> inputFiles)
    {
        std::vector<std::string> treePathList;
        std::stringstream ss(treePaths);
        std::string path;
        while (std::getline(ss, path, ',')) {
            treePathList.push_back(path);
        }
    
        std::string treeName;
        if (!treePathList.empty()) {
            size_t pos = treePathList[0].rfind('/');
            if (pos != std::string::npos) {
                treeName = treePathList[0].substr(pos + 1);
            }
        }
    
        std::unique_ptr<TChain> chain(new TChain(treeName.c_str()));

        for (const auto& inputFile : inputFiles) {
            for (const auto& treePath : treePathList) {
                std::string fullPath = inputFile + "/" + treePath;
                chain->Add(fullPath.c_str());
            }
        }
        return chain;
    }
    void BackProjectA(TRandom3* myRNG, TH2D* errortable, TH2D& h, TChain* chain, float length, bool isW)
    {
        double ep,et;
        float x0_array[3], theta, phi, step;
        chain->SetBranchAddress("x0", &x0_array);// unit: mm, range: +/-150mm (300mm detector)
        chain->SetBranchAddress("theta", &theta);
        chain->SetBranchAddress("phi", &phi);

        if (isW) {
            float w;
            chain->SetBranchAddress("w", &w);

            Long64_t nEntries = chain->GetEntries();
            for (Long64_t i = 0; i < nEntries; i++) {
                chain->GetEntry(i);
                if (cos(theta) <= 0) {
                    continue; // Skip invalid theta values
                }
                step = length * 1000 / cos(theta);
                errortable->GetRandom2(ep, et, myRNG);
                phi += ep;
                theta += et;

                if(phi < -pi_f) {
                    phi = 2*pi_f+phi;
                }
                else if(phi > pi_f) {
                    phi = phi-2*pi_f;
                }

                if(theta < 0) {
                    theta = 1e-6;
                }
                else if(theta > pi_f/2) {
                    theta = pi_f/2-1e-6;
                }
                h.Fill(
                    x0_array[0] + step * sin(theta) * cos(phi),
                    x0_array[1] + step * sin(theta) * sin(phi),
                    w
                );
            }
            for (int ix = 1; ix <= h.GetNbinsX(); ++ix) {
                for (int iy = 1; iy <= h.GetNbinsY(); ++iy) {
                    int bin = h.GetBin(ix, iy);
                    double content = h.GetBinContent(bin);
                    h.SetBinError(bin, std::sqrt(content));
                }
            }
        }
        else{
        Long64_t nEntries = chain->GetEntries();
        for (Long64_t i = 0; i < nEntries; i++) {
            chain->GetEntry(i);
            if (cos(theta) <= 0) {
                    continue; // Skip invalid theta values
                }
            step = length * 1000 / cos(theta);
            errortable->GetRandom2(ep, et, myRNG);
            phi += ep;
            theta += et;

            if(phi < -pi_f) {
                phi = 2*pi_f+phi;
            }
            else if(phi > pi_f) {
                phi = phi-2*pi_f;
            }

            if(theta < 0) {
                theta = 1e-6;
            }
            else if(theta > pi_f/2) {
                theta = pi_f/2-1e-6;
            }
            h.Fill(
                x0_array[0] + step * sin(theta) * cos(phi),
                x0_array[1] + step * sin(theta) * sin(phi)
            );
        }}
    }

    ManagedChain ApplyEulerToChain(TChain* chain,
                                    const std::string& treeName,
                                    std::vector<double> AngleToTurn, bool isW)
    {
        // Replace '/' in treeName for filename
        std::string safeFileName = treeName;
        std::replace(safeFileName.begin(), safeFileName.end(), '/', '_');
        std::string tmpFile = "../data/tmp_euler_" + safeFileName + ".root";

        // Manually create TTree for transformed results
        TFile f(tmpFile.c_str(), "RECREATE");
        TTree* tree = new TTree(treeName.c_str(), treeName.c_str());

        float x0[3], theta, phi, w;
        tree->Branch("x0",    x0,     "x0[3]/F");
        tree->Branch("theta", &theta, "theta/F");
        tree->Branch("phi",   &phi,   "phi/F");
        
        // Get x0 from original chain
        float orig_x0[3], orig_w, theta_tmp;
        chain->SetBranchAddress("x0", orig_x0);
        chain->SetBranchAddress("theta", &theta_tmp);
        if (isW) {
            tree->Branch("w",     &w,     "w/F");
            chain->SetBranchAddress("w", &orig_w);
        }

        ROOT::RDataFrame df(*chain);
        auto uvw{thetaphi2uvw(df)};
        auto tp_result{EulerTransform(uvw, AngleToTurn)}; // tp struct

        Long64_t nEntries = chain->GetEntries();
        for (Long64_t i = 0; i < nEntries; i++) {
            chain->GetEntry(i);
            if (theta_tmp < 0){
                continue;
            }
            std::copy(orig_x0, orig_x0 + 3, x0);
            if (isW) {
                w = orig_w;
            }
            theta = tp_result->theta->at(i);  // Use transformed theta
            phi   = tp_result->phi->at(i);    // Use transformed phi
            tree->Fill();
        }

        f.Write();
        f.Close();

        TChain* newChain = new TChain(treeName.c_str());
        newChain->Add(tmpFile.c_str());
        return ManagedChain(newChain, ChainWithTmpFile{tmpFile});
    }

    uvw* thetaphi2uvw(ROOT::RDataFrame& rdf) // Caller must manually free memory
    {
        // auto theta_col = rdf.Take<float>("theta");
        // auto phi_col = rdf.Take<float>("phi");

        auto theta_col = rdf.Define("theta_float", "static_cast<float>(theta)")
                .Take<float>("theta_float");
        auto phi_col = rdf.Define("phi_float", "static_cast<float>(phi)")
                .Take<float>("phi_float");
        // auto theta_col = rdf.Define("theta_float", "static_cast<float>(TransTheta)")
        //         .Take<float>("theta_float");
        // auto phi_col = rdf.Define("phi_float", "static_cast<float>(TransPhi)")
        //         .Take<float>("phi_float");

        std::vector<float>* u = new std::vector<float>;
        std::vector<float>* v = new std::vector<float>;
        std::vector<float>* w = new std::vector<float>;

        for (size_t i = 0; i < theta_col->size(); ++i) {
            float theta = (*theta_col)[i];
            float phi = (*phi_col)[i];

            // if (theta < 0){
            //     continue;
            // }
            if (theta > pi_f/2 || phi < -pi_f || phi > pi_f) {
                std::cerr << "Warning: theta = " << theta << " phi = " << phi << std::endl;
                throw std::runtime_error("Invalid theta or phi value");
            }
            

            float u_value = sin(theta) * cos(phi);
            float v_value = sin(theta) * sin(phi);
            float w_value = cos(theta);

            u->push_back(u_value);
            v->push_back(v_value);
            w->push_back(w_value);
        }

        auto* result = new uvw;
        result->u = u;
        result->v = v;
        result->w = w;
        return result;
    }

    tp* EulerTransform(uvw* Ddata, std::vector<double> AngleToTurn)
    {
        const auto u_set{Ddata->u};
        const auto v_set{Ddata->v};
        const auto w_set{Ddata->w};
        const float phi{static_cast<float>(AngleToTurn[0] * pi_f / 180.0)};
        const float theta{static_cast<float>(AngleToTurn[1] * pi_f / 180.0)};
        const float alpha{static_cast<float>(AngleToTurn[2] * pi_f / 180.0)};

        std::vector<float>* p = new std::vector<float>;
        std::vector<float>* t = new std::vector<float>;
        auto* result = new tp;
        // phi=gamma (rotation around z), theta=beta (around y), alpha=0 (around x)
        // const std::array<float, 3> Eulerrow0{cos(phi)*cos(theta), -sin(phi), cos(phi) * sin(theta)}; 
        // const std::array<float, 3> Eulerrow1{sin(phi) * cos(theta), cos(phi), sin(phi) * sin(theta)}; 
        // const std::array<float, 3> Eulerrow2{-sin(theta), 0.0f, cos(theta)}; 
        const std::array<float, 3> Eulerrow0{cos(phi)*cos(theta)*cos(alpha)-sin(phi)*sin(alpha), -sin(phi)*cos(alpha)-cos(phi)*cos(theta)*sin(alpha), cos(phi) * sin(theta)}; 
        const std::array<float, 3> Eulerrow1{sin(phi)*cos(theta)*cos(alpha)+cos(phi)*sin(alpha), cos(phi)*cos(alpha)-sin(phi)*cos(theta)*sin(alpha), sin(phi) * sin(theta)}; 
        const std::array<float, 3> Eulerrow2{-sin(theta)*cos(alpha), sin(theta)*sin(alpha), cos(theta)}; 

        // Ensure all vectors have the same size
        const size_t data_size = u_set->size();
        if (v_set->size() != data_size || w_set->size() != data_size) {
            throw std::runtime_error("Vector sizes mismatch");
        }

        // Iterate over all elements
        for (size_t i = 0; i < data_size; ++i) {
            // Get original coordinates
            const float u = u_set->at(i);
            const float v = v_set->at(i);
            const float w = w_set->at(i);
            
            // Compute matrix transformation (dot product)
            const float new_u = Eulerrow0[0] * u + Eulerrow0[1] * v + Eulerrow0[2] * w;
            const float new_v = Eulerrow1[0] * u + Eulerrow1[1] * v + Eulerrow1[2] * w;
            const float new_w = Eulerrow2[0] * u + Eulerrow2[1] * v + Eulerrow2[2] * w;

            if(w < 0.0) {
                std::cerr << "Warning: w = " << new_w << std::endl;
                throw std::runtime_error("Invalid theta_m value");
            }

            p->push_back(atan2(new_v, new_u)); // Use atan2 for quadrant handling; phi range: [-pi, pi]
            t->push_back(acos(new_w));// Compute zenith angle
        }

        result->phi = p;
        result->theta = t;
        return result;

    }



} // namespace Musae::AnaOpacity
