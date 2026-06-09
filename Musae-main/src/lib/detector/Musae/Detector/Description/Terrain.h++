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

#include "Mustard/Detector/Description/DescriptionWithCacheBase.h++"

#include "muc/array"

#include <memory>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

extern "C" {
struct turtle_stack;
struct turtle_projection;
} // extern "C"

namespace Musae::Detector::Description {

class Terrain final : public Mustard::Detector::Description::DescriptionWithCacheBase<Terrain> {
    friend Mustard::Env::Memory::SingletonInstantiator;

private:
    Terrain();
    ~Terrain() = default;

public:
    auto ElevationDataPath() const -> const auto& { return *fElevationDataPath; }
    auto Projection() const -> const auto& { return *fProjection; }
    auto ReferenceLatitude() const -> auto { return *fReferenceLatitude; }
    auto ReferenceLongitude() const -> auto { return *fReferenceLongitude; }
    auto ReferenceElevation() const -> auto { return *fReferenceElevation; }
    auto MinLatitude() const -> auto { return *fMinLatitude; }
    auto MaxLatitude() const -> auto { return *fMaxLatitude; }
    auto MinLongitude() const -> auto { return *fMinLongitude; }
    auto MaxLongitude() const -> auto { return *fMaxLongitude; }
    auto MinZ() const -> auto { return *fMinZ; }
    auto NVertex() const -> auto { return *fNVertex; }
    auto ReferenceXYZ() const -> auto { return *fReferenceXYZ; }
    auto CsvLoad() const -> auto { return *fCsvloading; }
    auto CsvNx() const -> auto { return *fCsvNx; }
    auto CsvNy() const -> auto { return *fCsvNy; }
    auto Material() const -> auto { return *fMaterial; }

    auto ElevationDataPath(std::string val) -> void { fElevationDataPath = std::move(val); }
    auto Projection(std::string val) -> void { fProjection = std::move(val); }
    auto ReferenceLatitude(double val) -> void { fReferenceLatitude = val; }
    auto ReferenceLongitude(double val) -> void { fReferenceLongitude = val; }
    auto ReferenceElevation(double val) -> void { fReferenceElevation = val; }
    auto MinLatitude(double val) -> void { fMinLatitude = val; }
    auto MaxLatitude(double val) -> void { fMaxLatitude = val; }
    auto MinLongitude(double val) -> void { fMinLongitude = val; }
    auto MaxLongitude(double val) -> void { fMaxLongitude = val; }
    auto MinZ(double val) -> void { fMinZ = val; }
    auto NVertex(int val) -> void { fNVertex = val; }
    auto CsvLoad(bool val)  -> void { fCsvloading = val; }
    auto CsvNx(int val)  -> void { fCsvNx = val; }
    auto CsvNy(int val)  -> void { fCsvNy = val; }
    auto Material(std::string val) -> void { fMaterial = std::move(val); }
    

    auto Elevation(double latitude, double longitude) const -> double;
    auto Project(double latitude, double longitude, double elevation) const -> muc::array3d;
    auto Unproject(muc::array3d xyz) const -> std::tuple<double, double, double>;

private:
    struct TurtleDeleter {
        auto operator()(turtle_stack* p) -> void;
        auto operator()(turtle_projection* p) -> void;
    };

private:
    auto CreateTurtleStack() const -> std::unique_ptr<turtle_stack, TurtleDeleter>;
    auto CreateTurtleProjection() const -> std::unique_ptr<turtle_projection, TurtleDeleter>;
    auto ProjectReferenceXYZ() const -> muc::array3d;

    auto ImportAllValue(const YAML::Node& node) -> void override;
    auto ExportAllValue(YAML::Node& node) const -> void override;

private:
    Simple<std::string> fElevationDataPath;
    Simple<std::string> fProjection;
    Simple<double> fReferenceLatitude;
    Simple<double> fReferenceLongitude;
    Simple<double> fReferenceElevation;
    Simple<double> fMinLatitude;
    Simple<double> fMaxLatitude;
    Simple<double> fMinLongitude;
    Simple<double> fMaxLongitude;
    Simple<double> fMinZ;
    Simple<bool> fCsvloading;
    Simple<int> fCsvNx;
    Simple<int> fCsvNy;
    Simple<std::string> fMaterial;


    Simple<int> fNVertex;

    Cached<std::unique_ptr<turtle_stack, TurtleDeleter>> fTurtleStack;
    Cached<std::unique_ptr<turtle_projection, TurtleDeleter>> fTurtleProjection;
    Cached<muc::array3d> fReferenceXYZ;
};

} // namespace Musae::Detector::Description
