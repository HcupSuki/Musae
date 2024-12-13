#include "Musae/GenCRMu/CLI.h++"
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

auto main(int argc, char* argv[]) -> int {
    Musae::GenCRMu::CLI cli;
    Mustard::Env::MPIEnv env{argc, argv, cli};

    Mustard::UseXoshiro<512> random;
    cli.SeedRandomIfFlagged();

    // Open file
    const auto filePath{Mustard::MPIX::ParallelizePath(cli.OutputFilePath()).generic_string()};
    TFile file{filePath.c_str(), cli.OutputFileMode().c_str()};
    if (not file.IsOpen()) {
        return EXIT_FAILURE;
    }

    // Generate events
    Musae::GenCRMu::Generator generator{cli};
    Mustard::MasterPrintLn("Generating {} weighted events...", cli.NEvent());
    long double weightSum{};
    long double weight2Sum{};
    Mustard::Data::Output<Musae::GenCRMu::CRMuEvent> dataOut{"CRMu", "Cosmic ray muon event"};
    Mustard::MPIX::Executor<long long>{"Generation", "Sample"}
        .Execute(cli.NEvent(),
                 [&](auto) {
                     const auto event{generator()};
                     const auto weight{event->GetPrimaryVertex()->GetWeight()};
                     weightSum += weight;
                     weight2Sum += muc::pow<2>(weight);
                     dataOut.Fill(Musae::GenCRMu::BackProjection(*event, cli.PrimaryZ()));
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
            cli.NEvent(), nEff, ok ? "(OK)" : "(**BAD**)",
            ok ? "" : "(**INACCURATE**) ", weightSum, weightSumError,
            ok ? "" : "(**INACCURATE**) ", estimatedSeconds, estimatedSecondsError,
            ok ? "" : "(**INACCURATE**) ", estimatedSeconds / cli.NEvent(), estimatedSecondsError / cli.NEvent(),
            ok ? "" : "(**INACCURATE**) ", cli.NEvent() / estimatedSeconds, cli.NEvent() / muc::pow<2>(estimatedSeconds) * estimatedSecondsError)};
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
