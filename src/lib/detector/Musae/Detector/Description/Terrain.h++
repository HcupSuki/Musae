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
    auto RockElement() const -> auto { return *fRockElement; }
    auto RockNAtom() const -> auto { return *fRockNAtom; }
    auto RockDensity() const -> auto { return *fRockDensity; }
    auto ReferenceXYZ() const -> auto { return *fReferenceXYZ; }

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
    auto RockElement(std::vector<std::string> val) -> void { fRockElement = std::move(val); }
    auto RockNAtom(std::vector<int> val) -> void { fRockNAtom = std::move(val); }
    auto RockDensity(double val) -> void { fRockDensity = val; }

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

    Simple<int> fNVertex;
    Simple<std::vector<std::string>> fRockElement;
    Simple<std::vector<int>> fRockNAtom;
    Simple<double> fRockDensity;

    Cached<std::unique_ptr<turtle_stack, TurtleDeleter>> fTurtleStack;
    Cached<std::unique_ptr<turtle_projection, TurtleDeleter>> fTurtleProjection;
    Cached<muc::array3d> fReferenceXYZ;
};

} // namespace Musae::Detector::Description
