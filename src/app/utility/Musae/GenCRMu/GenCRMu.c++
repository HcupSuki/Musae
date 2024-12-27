#include "Musae/GenCRMu/CLI.h++"
#include "Musae/GenCRMu/GenCRMu.h++"
#include "Musae/GenCRMu/Generator.h++"

#include "Mustard/Data/Output.h++"
#include "Mustard/Env/MPIEnv.h++"
#include "Mustard/Extension/MPIX/DataType.h++"
#include "Mustard/Extension/MPIX/Execution/Executor.h++"
#include "Mustard/Extension/MPIX/ParallelizePath.h++"
#include "Mustard/Utility/MakeTextTMacro.h++"
#include "Mustard/Utility/PrettyLog.h++"
#include "Mustard/Utility/Print.h++"
#include "Mustard/Utility/UseXoshiro.h++"

#include "ROOT/RDataFrame.hxx"
#include "TFile.h"
#include "TMacro.h"
#include "TROOT.h"

#include "G4Event.hh"
#include "G4SystemOfUnits.hh"

#include "mpi.h"

#include "muc/math"

#include "fmt/core.h"

#include <cmath>
#include <cstdlib>

namespace Musae::GenCRMu {

GenCRMu::GenCRMu() :
    Subprogram{"GenCRMu", "A utility program to generate weighted cosmic-ray muon events."} {}

auto GenCRMu::Main(int argc, char* argv[]) const -> int {
    CLI cli;
    cli->add_argument("n").help("Number of events to generate.").nargs(1).scan<'i', unsigned long long>();
    cli->add_argument("-o", "--output").help("Output file path.").required().nargs(1);
    cli->add_argument("--output-mode").help("Output file creation mode.").required().nargs(1).default_value("NEW");
    Mustard::Env::MPIEnv env{argc, argv, cli};

    Mustard::UseXoshiro<512> random;
    cli.SeedRandomIfFlagged();

    // Open file
    const auto filePath{Mustard::MPIX::ParallelizePath(cli->get("-o")).generic_string()};
    TFile file{filePath.c_str(), cli->get("--output-mode").c_str()};
    if (not file.IsOpen()) {
        return EXIT_FAILURE;
    }

    // Generate events
    const auto nEvent{cli->get<unsigned long long>("-n")};
    Generator generator{cli};
    Mustard::MasterPrintLn("Generating {} weighted events...", nEvent);
    long double weightSum{};
    long double weight2Sum{};
    Mustard::Data::Output<CRMuEvent> dataOut{"CRMu", "Cosmic ray muon event"};
    Mustard::MPIX::Executor<unsigned long long>{"Generation", "Sample"}
        .Execute(nEvent,
                 [&](auto) {
                     const auto event{generator()};
                     const auto weight{event->GetPrimaryVertex()->GetWeight()};
                     weightSum += weight;
                     weight2Sum += muc::pow<2>(weight);
                     dataOut.Fill(BackProjection(*event, cli.PrimaryZ()));
                 });
    dataOut.Write();
    MPI_Allreduce(MPI_IN_PLACE, &weightSum, 1, Mustard::MPIX::DataType(weightSum), MPI_SUM, MPI_COMM_WORLD);
    MPI_Allreduce(MPI_IN_PLACE, &weight2Sum, 1, Mustard::MPIX::DataType(weight2Sum), MPI_SUM, MPI_COMM_WORLD);

    // Report
    if (env.OnCommWorldMaster()) {
        const auto weightSumError{std::sqrt(weight2Sum)};
        const auto nEff{muc::pow<2>(weightSum) / weight2Sum};
        const auto ok{nEff >= 1000};
        constexpr auto hFlux{129 * hertz / m2};
        const auto estimatedSeconds{generator.EstimatedTime(weightSum, hFlux) / s};
        const auto estimatedSecondsError{generator.EstimatedTime(weightSumError, hFlux) / s};
        const auto info{fmt::format(
            "{} weighted events are generated, and N_eff = {:.2f} {}.\n"
            "- {}Equivelent to {:.6} +/- {:.3} unbiased events.\n"
            "- {}Correspond to {:.6} +/- {:.3} seconds wall time (estimated from 129 Hz/m2 horizontal flux), or\n"
            "- {}{:.6} +/- {:.3} seconds per weighted event, or\n"
            "- {}{:.6} +/- {:.3} weighted event per seconds.\n",
            nEvent, nEff, ok ? "(OK)" : "(**BAD**)",
            ok ? "" : "(**INACCURATE**) ", weightSum, weightSumError,
            ok ? "" : "(**INACCURATE**) ", estimatedSeconds, estimatedSecondsError,
            ok ? "" : "(**INACCURATE**) ", estimatedSeconds / nEvent, estimatedSecondsError / nEvent,
            ok ? "" : "(**INACCURATE**) ", nEvent / estimatedSeconds, nEvent / muc::pow<2>(estimatedSeconds) * estimatedSecondsError)};
        Mustard::Print("Generation completed, {}"
                       "(The above information will be saved to '{}')\n",
                       info, filePath);
        if (not ok) {
            Mustard::PrintWarning("N_eff TOO LOW. "
                                  "This generally means there are a few highly weighted events "
                                  "and THEY CAN BIAS THE ESTIMATIONS. "
                                  "All estimations should be considered inaccurate.");
        }
        Mustard::MakeTextTMacro(generator.Information() + info, "CRMuInfo")->Write();
    }

    return EXIT_SUCCESS;
}

} // namespace Musae::GenCRMu
