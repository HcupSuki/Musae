#include "Musae/AnaOpacity/AnaProcess.h++"

#include "CLHEP/Geometry/Point3D.h"
#include "CLHEP/Geometry/Transform3D.h"
#include "CLHEP/Geometry/Vector3D.h"
#include "CLHEP/Vector/ThreeVector.h"

namespace Musae::AnaOpacity {

    using namespace Mustard::MathConstant;
    using namespace std::string_literals;
    using namespace std;
    using UnitMap = std::unordered_map<std::string, double>;

    double unit_value(const std::string& unit_str, UnitMap unitmap)
    {
        auto it = unitmap.find(unit_str);
        if (it != unitmap.end()) {
            return it->second;
        }
        throw std::invalid_argument("Unknown unit: " + unit_str);
    }
    
    void DrawHistogram(TCanvas& c, TH2D& h, double minValue, double maxValue, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file)
    {
        gStyle->SetNumberContours(99);
        gStyle->SetTitleFontSize(0.035);
        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(99);
        style.Draw();

        c.SetTheta(90);
        c.SetPhi(rawHModel[3]); // 180 to fit the axis!

        h.SetMinimum(minValue);
        h.SetMaximum(maxValue);

        h.SetLineColor(h.GetFillColor());
        h.SetLineColorAlpha(kBlack, 0);
        h.GetXaxis()->SetTitle("#phi (rad)");
        h.GetYaxis()->SetTitle("#theta (rad)");
        h.SetStats(false);
        h.GetZaxis()->SetMaxDigits(0);
        h.GetZaxis()->SetNdivisions(105);
        h.SetTitleSize(0.035);
        h.GetZaxis()->SetTitleOffset(1.5);
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
        palette->SetX1NDC(0.85);
        palette->SetX2NDC(0.92);
        palette->SetY1NDC(0.73);
        palette->SetY2NDC(0.94);
        c.Modified();
        c.Update();

        if (file) {
            h.Write();
            c.Write();
        }
    }

    void DrawHistogram(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, std::vector<double> rawHModel, TFile* file)
    {
        gStyle->SetNumberContours(99);
        gStyle->SetTitleFontSize(0.035);
        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(99);
        style.Draw();

        c.SetTheta(90);
        c.SetPhi(rawHModel[3]); // 180 to fit the axis!

        h.SetLineColor(h.GetFillColor());
        h.SetLineColorAlpha(kBlack, 0);
        h.GetXaxis()->SetTitle("#phi (rad)");
        h.GetYaxis()->SetTitle("#theta (rad)");
        h.SetStats(false);
        h.GetZaxis()->SetMaxDigits(0);
        h.GetZaxis()->SetNdivisions(105);
        h.SetTitleSize(0.035);
        h.GetZaxis()->SetTitleOffset(1.5);
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
        palette->SetX1NDC(0.85);
        palette->SetX2NDC(0.92);
        palette->SetY1NDC(0.73);
        palette->SetY2NDC(0.94);


        // palette->SetY1NDC(0.65);
        // palette->SetY2NDC(0.97);
        // palette->SetX1NDC(0.9);
        // palette->SetX2NDC(0.935);

        // palette->SetY1NDC(0.62);
        // palette->SetY2NDC(0.93);
        // palette->SetX1NDC(0.01);
        // palette->SetX2NDC(0.055);
        c.Modified();
        c.Update();

        if (file) {
            h.Write();
            c.Write();
        }
    }

    void DrawHistogram_rec(TCanvas& c, TH2D& h, std::underlying_type_t<EColorPalette> palettetype, TFile* file)
    {
        c.cd();
        gStyle->SetNumberContours(99);

        TStyle style;
        style.SetPalette(palettetype);
        style.SetNumberContours(99);
        style.Draw();
        h.SetLineColorAlpha(kBlack, 0);
        h.GetXaxis()->SetTitle("#phi (rad)");
        h.GetYaxis()->SetTitle("#theta (rad)");
        h.SetStats(false);
        h.Draw("colz");
        c.Update();
        // h.SetMaximum(h.GetMaximum());

        // const auto palette{dynamic_cast<TPaletteAxis*>(h.GetListOfFunctions()->FindObject("palette"))};
        // palette->SetY1NDC(0.1);
        // palette->SetY2NDC(0.9);
        // c.Modified();
        // c.Update();

        TPaletteAxis* palette = dynamic_cast<TPaletteAxis*>(h.GetListOfFunctions()->FindObject("palette"));
    
        if (!palette) {
            std::cout << "Palette not found, creating a new one..." << std::endl;
            
            // Redraw histogram with z option to create palette
            h.Draw("colz");
            c.Update();

            // Retry palette lookup
            palette = dynamic_cast<TPaletteAxis*>(h.GetListOfFunctions()->FindObject("palette"));

            if (!palette) {
                // Create a palette manually as fallback
                Double_t x1 = 0.91;
                Double_t x2 = 0.95;
                Double_t y1 = 0.1;
                Double_t y2 = 0.9;
                palette = new TPaletteAxis(x1, y1, x2, y2, &h);
                h.GetListOfFunctions()->Add(palette);
                std::cout << "Created a new palette manually" << std::endl;
            }
        }
        
        // Safe to set palette attributes now
        if (palette) {
            palette->SetY1NDC(0.1);
            palette->SetY2NDC(0.9);
            c.Modified();
            c.Update();
        } else {
            std::cout << "Warning: Failed to create or find palette" << std::endl;
        }

        if (file) {
            h.Write();
            c.Write();
            c.Close();
        }
        else {
            std::cout << "File is null" << std::endl;
        }
    }

    void Csvout(TH2D& hOpacity, const char* CsvfilePath)
    {
        // Write table data to CSV file
        std::ofstream csvFile;
        csvFile.open(CsvfilePath);

        // Check if file opened successfully
        if (!csvFile.is_open()) {
            std::cerr << "Failed to open file: " << *CsvfilePath << std::endl;
            throw std::runtime_error{"Failed to open output file."};
        }

        double ta_d = hOpacity.GetNbinsY(), phi_d = hOpacity.GetNbinsX(), d_ta = hOpacity.GetYaxis()->GetBinWidth(1), d_phi = hOpacity.GetXaxis()->GetBinWidth(1);
        // Write CSV header
        csvFile << "u,v,w,bincontent,error\n";

        for (int j = 1; j <= ta_d; ++j) { // Iterate over zenith angles
        for (int i = 1; i <= phi_d; ++i) {// Iterate over azimuthal angles

            Double_t binContent = hOpacity.GetBinContent(i,j);
            Double_t error = hOpacity.GetBinError(i,j);
            Double_t u= sin(0 + d_ta*j - d_ta/2)*cos(-pi + d_phi*i - d_phi/2);
            Double_t v= sin(0 + d_ta*j - d_ta/2)*sin(-pi + d_phi*i - d_phi/2);
            Double_t w= cos(0 + d_ta*j - d_ta/2);
            csvFile << u << "," << v << "," << w << "," << binContent << "," << error << "\n";
            
        }}
    }

    vector<string> split(const string& s)
    {
        vector<string> tokens;
        string token;
        istringstream iss(s);
        while (iss >> token) {
            tokens.push_back(token);
        }
        return tokens;
    }

    map<string, vector<double>> MaterialLoad(const string& filename, 
        const vector<string>& selectedColumns) 
    {
        map<string, vector<double>> result;
        ifstream file(filename);
        string line;
        vector<string> headers;

        // Find header row
        while (getline(file, line)) {
        if (line.find("T") != string::npos && line.find("p") != string::npos) {
        headers = split(line);
        break;
        }
        }

        if (headers.empty()) {
        throw runtime_error("Header not found in file");
        }

        // Validate requested columns and get indices
        vector<size_t> columnIndices;
        for (const auto& col : selectedColumns) {
        auto it = find(headers.begin(), headers.end(), col);
        if (it == headers.end()) {
        throw runtime_error("Column '" + col + "' not found in header");
        }
        columnIndices.push_back(distance(headers.begin(), it));
        }

        // Read data rows
        while (getline(file, line)) {
        // Skip comment and empty lines
        if (line.empty() || line.find("***") != string::npos) {
        continue;
        }

        vector<string> tokens = split(line);
        if (tokens.size() != headers.size()) {
        continue; // Skip malformed lines
        }

        // Extract requested column data
        for (size_t i = 0; i < selectedColumns.size(); ++i) {
        try {
        double value = stod(tokens[columnIndices[i]]);
        result[selectedColumns[i]].push_back(value);
        } catch (const exception& e) {
        cerr << "Error parsing value: " << tokens[columnIndices[i]] 
        << " in column " << selectedColumns[i] << endl;
        }
        }
        }

        return result;
    }

    std::pair<double, double>* lqpoint(long long int index, double Ecut_value, const map<string, vector<double>>& data) {
        // Get T and CSDARange data references
        const vector<double>& T = data.at("T");
        const vector<double>& CSDARange = data.at("CSDARange");

        if (index < 1 || index >= static_cast<long long>(T.size()) - 1) {
            std::cerr << "Index out of range, index: " << index << " data_size = " << T.size() -1 << std::endl;
            throw runtime_error("Index out of range");
        }

        
        // Extract data from three neighboring points
        vector<double> x, y;
        for (int i = index-1; i <= index+1; ++i) {
            x.push_back(T[i]);
            y.push_back(CSDARange[i]);
        }
    
        // Compute linear fit parameters via least squares
        double sum_x = 0, sum_y = 0, sum_x2 = 0, sum_xy = 0;
        const int n = 3; // Use three points
        
        for (int i = 0; i < n; ++i) {
            sum_x += x[i];
            sum_y += y[i];
            sum_x2 += x[i] * x[i];
            sum_xy += x[i] * y[i];
        }
    
        // Compute slope b and intercept a
        double denominator = n * sum_x2 - sum_x * sum_x;
        if (denominator == 0) {
            throw runtime_error("Linear regression failed (zero denominator)");
        }
        
        double b = (n * sum_xy - sum_x * sum_y) / denominator;
        double a = (sum_y - b * sum_x) / n;

        std::pair<double, double>* result = new std::pair<double, double>(a + b * Ecut_value, b);
    
        // Return interpolation result
        return result;
    }

    std::pair<double, double>* SearchRange(const string& filename, const double Ecut, const double error) // Input Ecut; note unit conversion
    {
        vector<string> Head = {"T", "CSDARange"}; // Read energy T (MeV) and CSDARange (g/cm^2)

        auto data = MaterialLoad(filename, Head);
        auto data_size{data.begin()->second.size()};


        double distance{abs(data["T"][0] - Ecut)};
        long long int index{0};
        for (size_t i = 1; i < data_size; ++i) {
            if (abs(data["T"][i] - Ecut) < distance) { // Find energy closest to E_cut
                distance = abs(data["T"][i] - Ecut);
                index = i;
            }
        }
        if (index == 0) {
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            // std::cerr << "Ecut is smaller than the minimum energy in the table!!" << std::endl;
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            std::cerr << "Ecut = " << Ecut << ", min energy in table = " << data["T"][0] << std::endl;
            throw runtime_error("Ecut is smaller than the minimum energy in the table");
        } 
        else if (index == static_cast<long long>(data_size) - 1) {
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            // std::cerr << "Ecut is smaller than the minimum energy in the table!!" << std::endl;
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            throw runtime_error("Ecut is larger than the maximum energy in the table");
        }
        auto Range{lqpoint(index, Ecut,data)};
        Range->second *= error; // Multiply slope by error to get Range uncertainty
        return Range;
           
    }

    std::pair<double, double>* Esearch(const double theta, const double survival_rate, const double error, std::vector<std::vector<double>> &data)
    {
        map<string, vector<double>> result;
        double distance{abs(data[0][0] - theta)};
        double theta0{data[0][0]};
        long long int refindex{0};
        long long int index{0};
        size_t data_size{data.size()};
        bool first{true};
        for (size_t i = 1; i < data_size; ++i) {// Find theta0 closest to theta
            if (abs(data[i][0] - theta) < distance) { 
                distance = abs(data[i][0] - theta);
                theta0 = data[i][0];
            }
        }
        for (size_t i = 0; i < data_size; ++i) { // Find surrate index closest to surrate
            if(data[i][0] != theta0) {
                continue;
            }
            if(first) {
                first = false;
                distance = abs(data[i][2] - survival_rate);
            }
            result["T"].push_back(data[i][2]);
            result["CSDARange"].push_back(data[i][4]);
            if (abs(data[i][2] - survival_rate) <= distance) { 
                distance = abs(data[i][2] - survival_rate);
                index = refindex;
            }
            refindex++;
        }
        vector<size_t> indices(result["T"].size());
        iota(indices.begin(), indices.end(), 0);
        sort(indices.begin(), indices.end(), 
            [&](size_t a, size_t b) { return result["T"][a] < result["T"][b]; });
        vector<double> sorted_T, sorted_CSDARange;
        sorted_T.reserve(result["T"].size());
        sorted_CSDARange.reserve(result["CSDARange"].size());
        for(auto idx : indices) {
            sorted_T.push_back(result["T"][idx]);
            sorted_CSDARange.push_back(result["CSDARange"][idx]);
        }
        result["T"] = move(sorted_T);
        result["CSDARange"] = move(sorted_CSDARange);
        // Update index to sorted position
        for(size_t i = 0; i < indices.size(); ++i) {
            if(indices[i] == static_cast<size_t>(index)) {
                index = i;
                break;
            }
        }
        if (index == 0 || index == static_cast<long long int>(result["T"].size()-1)) {
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            std::cerr << "survival_rate has reach its table limit, check your flux table or filiter" << std::endl;
            std::cerr << "survival_rate = " << survival_rate << ", theta0 = " << theta0 << ", index = " << index << std::endl;
            std::pair<double, double>* E{new std::pair<double, double> (0, 0)};
            return E; // Return empty pair: no suitable Ecut found
            // std::cerr << "++++++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
            // throw runtime_error("survival_rate is too small, check your flux table or bin width");
        }
        // std::cout << "Esearch: index = " << index << ", theta0 = " << theta0 << ", survival_rate = " << survival_rate << std::endl;
        auto E{lqpoint(index, survival_rate,result)};// Linear interpolation: use survival_rate to find E
        E->second *= error; // Multiply slope by error to get Ecut uncertainty
        return E;
    }

    std::pair<TH2D*, TH2D*>* SerachEandR(const TH2D& ref_h, const std::string& materialname, const std::string& fluxname, double unit, std::string search_mode) 
    {
        auto Nx{ref_h.GetNbinsX()};
        auto Ny{ref_h.GetNbinsY()};
        int minus_count{0};
        int derivative_count{0};

        std::vector<std::vector<double>> fluxdata; // Columns: "theta","dtheta","surrate","initialGuess","Ecut"
        std::ifstream file(fluxname);
        if (!file.is_open()) {
            std::cerr << "Error opening file!" << std::endl;
            throw std::runtime_error("File not found.");
        }

        std::string header;
        std::getline(file, header);

        std::string line;
        while (std::getline(file, line)) {
            std::vector<double> row;
            std::stringstream ss(line);
            std::string cell;

            while (std::getline(ss, cell, ',')) {
                row.push_back(stod(cell));
            }
            fluxdata.push_back(row);
        }
        // Create result histograms
        TH2D* result_h = dynamic_cast<TH2D*>(ref_h.Clone("result_h"));
        result_h->SetMaximum(-1111);
        result_h->SetMinimum(-1111);
        result_h->Reset();
        result_h->SetDirectory(nullptr);

        TH2D* result_E = dynamic_cast<TH2D*>(ref_h.Clone("result_E"));
        result_E->SetMaximum(-1111);
        result_E->SetMinimum(-1111);
        result_E->Reset();
        result_E->SetDirectory(nullptr);

        // Process each bin
        for (Int_t i = 1; i <= Nx; ++i) {
            for (Int_t j = 1; j <= Ny; ++j) {
                auto survival_rate{ref_h.GetBinContent(i, j)};
                auto survival_rate_error{ref_h.GetBinError(i, j)};
                auto theta{ref_h.GetYaxis()->GetBinCenter(j)};
                if(survival_rate >= 1 || survival_rate <= 0) {
                    continue;
                }
                auto E_cut{Esearch(theta, survival_rate, ref_h.GetBinError(i, j), fluxdata)};
                // Compute and store Range
                if (E_cut->first == 0) {
                    result_E->SetBinContent(i, j, 0);
                    result_E->SetBinError(i, j, 0);
                    result_h->SetBinContent(i, j, 0);
                    result_h->SetBinError(i, j, 0);
                    continue;
                }
                auto range{SearchRange(materialname, E_cut->first * unit, E_cut->second * unit)};

                if(survival_rate-survival_rate_error < 0 || search_mode == "Derivative" || Esearch(theta, survival_rate-survival_rate_error, ref_h.GetBinError(i, j), fluxdata)->first <= 2  || Esearch(theta, survival_rate+survival_rate_error, ref_h.GetBinError(i, j), fluxdata)->first == 0)
                {
                    result_E->SetBinContent(i, j, E_cut->first * unit);
                    result_E->SetBinError(i, j, E_cut->second * unit);
                    result_h->SetBinContent(i, j, range->first);
                    result_h->SetBinError(i, j, range->second);
                    derivative_count++;
                }
                else if (search_mode == "Minus") {
                    auto E_cut_upper{Esearch(theta, survival_rate+survival_rate_error, ref_h.GetBinError(i, j), fluxdata)};
                    auto E_cut_lower{Esearch(theta, survival_rate-survival_rate_error, ref_h.GetBinError(i, j), fluxdata)};
                    auto range_upper{SearchRange(materialname, E_cut_upper->first * unit, E_cut_upper->second * unit)};
                    auto range_lower{SearchRange(materialname, E_cut_lower->first * unit, E_cut_lower->second * unit)};
                    result_E->SetBinContent(i, j, E_cut->first * unit);
                    result_h->SetBinContent(i, j, range->first);
                    double E_cut_error = std::abs((E_cut_upper->first * unit - E_cut_lower->first * unit)) / 2.0;
                    double range_error = std::abs((range_upper->first - range_lower->first)) / 2.0;
                    result_E->SetBinError(i, j, E_cut_error);
                    result_h->SetBinError(i, j, range_error);
                    minus_count++;
                }else {
                    throw std::invalid_argument("Invalid search mode: " + search_mode);
                }
                result_E->SetBinContent(i, j, E_cut->first * unit);
                result_E->SetBinError(i, j, E_cut->second * unit);
                result_h->SetBinContent(i, j, range->first);
                result_h->SetBinError(i, j, range->second);
        }}
        result_h->SetNameTitle("Opacity", "Opacity (g/cm^{2})");
        result_h->SetXTitle("#phi (rad)");
        result_h->SetYTitle("#theta (rad)");
        result_E->SetNameTitle("Emin", "E_{min} (MeV)");
        result_E->SetXTitle("#phi (rad)");
        result_E->SetYTitle("#theta (rad)");
        std::pair<TH2D*, TH2D*>* result = new std::pair<TH2D*, TH2D*>(result_h, result_E);
        std::cout << "Minus count: " << minus_count << ", Derivative count: " << derivative_count << std::endl;
        return result;
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

            if (theta < 0){
                continue;
            }
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

    
    // std::pair<TH2D*, TH2D*>* SerachEcut(ROOT::RDF::RInterface<ROOT::Detail::RDF::RLoopManager, void> rdf_m, const TH2D& ref_h, const std::string& materialname, int min_entries, double unit) 
    // {
    //     // Get data columns
    //     // auto theta_col = rdf.Take<double>("theta");
    //     // auto phi_col = rdf.Take<double>("phi");
    //     // auto energy_col = rdf.Take<double>("Ek0");
    //     // auto weight_col = rdf.Take<double>("w");

    //     // auto theta_col = rdf.Take<float>("theta");
    //     // auto phi_col = rdf.Take<float>("phi");
    //     // auto energy_col = rdf.Take<float>("Ek0");
    //     // auto weight_col = rdf.Take<float>("w");

    //     auto theta_col = rdf_m.Define("theta_float", "static_cast<float>(TransTheta)")
    //              .Take<float>("theta_float");
    //     auto phi_col = rdf_m.Define("phi_float", "static_cast<float>(TransPhi)")
    //              .Take<float>("phi_float");
    //     auto energy_col = rdf_m.Define("energy_float", "static_cast<float>(Ek0)")
    //              .Take<float>("energy_float");
    //     auto weight_col = rdf_m.Define("weight_float", "static_cast<float>(w)")
    //              .Take<float>("weight_float");
    //     unit = 1;// Energy unit fixed to MeV
    //     // Create result histograms
    //     TH2D* result_h = dynamic_cast<TH2D*>(ref_h.Clone("result_h"));
    //     result_h->SetMaximum(-1111);
    //     result_h->SetMinimum(-1111);
    //     result_h->Reset();
    //     result_h->SetDirectory(nullptr);

    //     TH2D* result_E = dynamic_cast<TH2D*>(ref_h.Clone("result_h"));
    //     result_E->SetMaximum(-1111);
    //     result_E->SetMinimum(-1111);
    //     result_E->Reset();
    //     result_E->SetDirectory(nullptr);

    //     // Bin data by (phi, theta)
    //     std::unordered_map<int, BinData> bin_map;
    //     size_t binmax{0};
    //     // Fill data into corresponding bins
    //     for (size_t i = 0; i < theta_col->size(); ++i) {
    //         double theta = (*theta_col)[i];
    //         double phi = (*phi_col)[i];
    //         double energy = (*energy_col)[i];
    //         double weight = (*weight_col)[i];

    //         size_t bin = ref_h.FindFixBin(phi, theta);
        
    //         if (bin > binmax) binmax = bin;
    //         bin_map[bin].energies.push_back(energy);
    //         bin_map[bin].weights.push_back(weight);
    //         bin_map[bin].total_weight += weight;
    //     }

    //     // Process each bin
    //     for (size_t bin = 1; bin <= binmax; ++bin) {
    //         auto it = bin_map.find(bin);
    //         if (it == bin_map.end()) continue;

    //         const auto& bin_data = it->second;
    //         double target_survival = ref_h.GetBinContent(bin);
    //         // Skip invalid bins
    //         if (bin_data.total_weight < min_entries || 
    //             target_survival <= 0.0 || 
    //             target_survival >= 1.0) {
    //             continue;
    //         }

    //         // Create energy list sorted by weight
    //         std::vector<std::pair<double, double>> sorted_events;
    //         for (size_t i = 0; i < bin_data.energies.size(); ++i) {
    //             sorted_events.emplace_back(bin_data.energies[i], bin_data.weights[i]);
    //         }
    //         std::sort(sorted_events.begin(), sorted_events.end());

    //         // Compute required total weight
    //         double required_weight {bin_data.total_weight * target_survival};
    //         double accumulated {0.0};
    //         double E_cut {0.0};

    //         // Reverse scan (high to low energy)
    //         for (auto rit = sorted_events.rbegin(); rit != sorted_events.rend(); ++rit) {
    //             accumulated += rit->second;
    //             if (accumulated >= required_weight) {
    //                 E_cut = rit->first;
    //                 break;
    //             }
    //         }

    //         // Compute and store Range
    //         double range = SerachRange(materialname, E_cut * unit);
    //         result_E->SetBinContent(bin, E_cut * unit);
    //         result_h->SetBinContent(bin, range);
    //     }
    //     result_h->SetNameTitle("Range (g/cm2)_old", "Range (g/cm2)_old");
    //     result_h->SetXTitle("#phi (rad)");
    //     result_h->SetYTitle("#theta (rad)");
    //     result_E->SetNameTitle("E_min (MeV)_old", "E_min (MeV)_old");
    //     result_E->SetXTitle("#phi (rad)");
    //     result_E->SetYTitle("#theta (rad)");
    //     std::pair<TH2D*, TH2D*>* result = new std::pair<TH2D*, TH2D*>(result_h, result_E);
    //     return result;
    //     }
    void AError(tp* Adata, TRandom3* myRNG, TH2D* errortable)
    {
        size_t data_size{Adata->phi->size()};
        for (size_t i = 0; i < data_size; ++i) {
            double ep,et;
            double phi = (*Adata->phi)[i];
            double theta = (*Adata->theta)[i];

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

            (*Adata->phi)[i] = phi;
            (*Adata->theta)[i] = theta;
        }
    }

    std::vector<std::vector<double>> GenerateGaussianKernel(int size, double sigma)// size must be odd
    {
        int radius{(size-1) / 2};
        std::vector<std::vector<double>> kernel(size, std::vector(size, 0.0));
        double sum{0.0};

        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                int x = i - radius;
                int y = j - radius;
                double exponent = -(x*x + y*y) / (2.0 * sigma * sigma);
                kernel[i][j] = exp(exponent) / (2 * M_PI * sigma * sigma);
                sum += kernel[i][j];
            }
        }

        for (int i = 0; i < size; ++i) {
            for (int j = 0; j < size; ++j) {
                kernel[i][j] /= sum;
            }
        }

        return kernel;
    }

    void Significance(TH2D& h_original, TH2D& h_reference, TH2D& h_out)
    {
        Int_t Nx{h_original.GetNbinsX()};
        Int_t Ny{h_original.GetNbinsY()};
        if (Nx != h_reference.GetNbinsX() || Ny != h_reference.GetNbinsY()) {
            throw std::invalid_argument("Original and reference histograms must have the same dimensions.");
        }

        for(Int_t ix{1}; ix <= Nx; ++ix) {
            for (Int_t iy{1}; iy <= Ny; ++iy) {
                double original_content{h_original.GetBinContent(ix, iy)};
                double reference_content{h_reference.GetBinContent(ix, iy)};
                double original_error{h_original.GetBinError(ix, iy)};
                double reference_error{h_reference.GetBinError(ix, iy)};

                if (reference_content == 0 || original_content == 0) {
                    h_out.SetBinContent(ix, iy, 0);
                    h_out.SetBinError(ix, iy, 0);
                    continue;
                }

                double significance{(original_content - reference_content) / 
                                      sqrt(original_error * original_error + 
                                           reference_error * reference_error)};
                h_out.SetBinContent(ix, iy, significance);
            }
        }
    }


    void MeanFilter(TH2D* h_original, TH2D& h_out, int filter_size)// Convolution kernel: filter_size x filter_size
    {
        if (filter_size % 2 == 0) {
            throw std::invalid_argument("Size must be odd.");
        }
        const int range{(filter_size - 1) / 2};
        const int Nx{h_original->GetNbinsX()};
        const int Ny{h_original->GetNbinsY()};
        bool isvalid{true};
        for (int ix{1}; ix <= Nx; ix++) {
            for (int iy{1}; iy <= Ny; iy++) {
                
                double sum{0};
                double error_sum{0};

                for (int dx{-range}; dx <= range; dx++) {
                    for (int dy{-range}; dy <= range; dy++) {
                        int nx{ix + dx};
                        int ny{iy + dy};

                        if (ny < 1 || ny > Ny)
                        {
                            isvalid = false;
                            break;
                        }
                        else if (nx < 1)
                        {
                            nx = Nx + nx;
                        }
                        else if (nx > Nx)
                        {
                            nx = nx - Nx;
                        }
                        double bincontent{h_original->GetBinContent(nx, ny)};
                        if (bincontent <= 0)
                        {
                            isvalid = false;
                            break;
                        }
                        sum += bincontent;
                        error_sum += pow(h_original->GetBinError(nx, ny), 2);
                    }
                }
                if (isvalid) {
                    h_out.SetBinContent(ix, iy, sum / (filter_size*filter_size));
                    h_out.SetBinError(ix, iy, sqrt(error_sum) / (filter_size*filter_size));
                }
                isvalid = true; 
            }
        }
    }

    TH2D* GaussianFilter(TH2D* h_original,const int filter_size,const double sigma)
    {
        if (filter_size % 2 == 0) {
            throw std::invalid_argument("Size must be odd.");
        }

        const auto kernel = GenerateGaussianKernel(filter_size, sigma);
        const int range{(filter_size - 1) / 2};
        const int Nx{h_original->GetNbinsX()};
        const int Ny{h_original->GetNbinsY()};
        bool isvalid{true};

        TH2D* h_filtered = (TH2D*)h_original->Clone("GaussianFilter");
        h_filtered->Reset();

        for (int ix = 1; ix <= Nx; ++ix) {
            for (int iy = 1; iy <= Ny; ++iy) {
                double sum{0};
                double error_sum{0};

                for (int dx{-range}; dx <= range; dx++) {
                    for (int dy{-range}; dy <= range; dy++) {
                        int nx{ix + dx};
                        int ny{iy + dy};

                        if (ny < 1 || ny > Ny)
                        {
                            isvalid = false;
                            break;
                        }
                        else if (nx < 1)
                        {
                            nx = Nx + nx;
                        }
                        else if (nx > Nx)
                        {
                            nx = nx - Nx;
                        }
                        double bincontent{h_original->GetBinContent(nx, ny)};
                        if (bincontent <= 0)
                        {
                            isvalid = false;
                            break;
                        }
                        int kx = dx + range;  
                        int ky = dy + range;
                        double weight = kernel[kx][ky];
                        sum += weight * bincontent;
                        error_sum += pow(weight * h_original->GetBinError(nx, ny), 2);
                    }
                }
                if (isvalid) {
                h_filtered->SetBinContent(ix, iy, sum);
                h_filtered->SetBinError(ix, iy, sqrt(error_sum));
                }
                isvalid = true; 
            }
        }
        return h_filtered;
    }

    TH2D* MedianFilter(TH2D* h_original, const int filter_size)
    {
        if (filter_size % 2 == 0) {
            throw std::invalid_argument("Size must be odd.");
        }

        TH2D* h_filtered = (TH2D*)h_original->Clone("MedianFilter");
        h_filtered->Reset();

        const int range{(filter_size - 1) / 2};
        const int Nx{h_original->GetNbinsX()};
        const int Ny{h_original->GetNbinsY()};
        bool isvalid{true};

        for (int ix{1}; ix <= Nx; ix++) {
            for (int iy{1}; iy <= Ny; iy++) {

                std::vector<double> values_error;
                std::vector<std::pair<double, int>> indexed_values;
                int iter{0};

                for (int dx{-range}; dx <= range; dx++) {
                    for (int dy{-range}; dy <= range; dy++) {
                        int nx{ix + dx};
                        int ny{iy + dy};

                        if (ny < 1 || ny > Ny)
                        {
                            isvalid = false;
                            break;
                        }
                        else if (nx < 1)
                        {
                            nx = Nx + nx;
                        }
                        else if (nx > Nx)
                        {
                            nx = nx - Nx;
                        }
                        double bincontent{h_original->GetBinContent(nx, ny)};
                        if (bincontent <= 0)
                        {
                            isvalid = false;
                            break;
                        }
                        indexed_values.push_back({bincontent, iter});
                        values_error.push_back(h_original->GetBinError(nx, ny));
                        iter++;
                    }
                }
                if (isvalid) {
                    std::sort(indexed_values.begin(), indexed_values.end(), 
                        [](const std::pair<double, int>& a, const std::pair<double, int>& b) {
                            return a.first < b.first;
                        });
                    const int mid{(filter_size * filter_size - 1)/2};
                    const double median{indexed_values[mid].first};
                    h_filtered->SetBinContent(ix, iy, median);
                    h_filtered->SetBinError(ix, iy, values_error[indexed_values[mid].second]);
                }
                indexed_values.clear();
                values_error.clear();
                isvalid = true;
            }
        }
        return h_filtered;
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

    // ROOT::RDF::RInterface<ROOT::Detail::RDF::RLoopManager, void> Inclination(ROOT::RDataFrame& rdf, std::vector<double> AngleToTurn) // AngleToTurn: [phi, theta] in degrees
    // {
    //     auto uvw{thetaphi2uvw(rdf)}; // Convert theta and phi to uvw vectors
    //     auto tp{EulerTransform(uvw, AngleToTurn)};// Apply Euler transform, convert back to theta and phi

    //     // std::vector<float> PhiData{*tp->phi};
    //     // std::vector<float> ThetaData{*tp->theta};

    //     // rdf.Define("TransPhi",
    //     //     [&PhiData](ULong64_t entry) { return PhiData[entry]; },
    //     //     {"rdfentry_"})
    //     //     .Define("TransTheta",
    //     //     [&ThetaData](ULong64_t entry) { return ThetaData[entry]; },
    //     //     {"rdfentry_"});

    //     auto rdf_m = rdf.Define("TransPhi",
    //         [tp](ULong64_t entry) { return (*tp->phi)[entry]; },
    //         {"rdfentry_"}
    //     ).Define("TransTheta",
    //         [tp](ULong64_t entry) { return (*tp->theta)[entry]; },
    //         {"rdfentry_"}
    //     );

    //     delete uvw->u;
    //     delete uvw->v;
    //     delete uvw->w;
    //     delete uvw;
    //     delete tp->phi;
    //     delete tp->theta;
    //     delete tp;

    //     return rdf_m;
    // }
    
} // namespace Musae::AnaOpacity
