#include <cassert>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Init.h>
#include <plog/Log.h>

using plog::error;
using plog::fatal;
using plog::info;

#include "G4BooleanSolid.hh"
#include "G4IntersectionSolid.hh"
#include "G4LogicalVolume.hh"
#include "G4PhysicalConstants.hh"
#include "G4Sphere.hh"
#include "G4SubtractionSolid.hh"
#include "G4SystemOfUnits.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VSolid.hh"

#include "config_path.h"

#include "U4GDML.h"
#include "U4Solid.h"
#include "U4Volume.h"

#include "s_csg.h"

namespace
{
inline constexpr char testGeomFile[] = SIMPHONY_TEST_GEOM_DIR "/sphere_phicut_quarter_shell.gdml";

enum ConvertOutcome
{
    CONVERT_REJECTED,
    CONVERT_ACCEPTED,
    CONVERT_ERROR
};

bool IsExpectedRejectSignal(int signal)
{
    return signal == SIGABRT || signal == SIGINT;
}

bool HasSinglePhiShell(const sn* nd, double outer_radius, double inner_radius)
{
    if (nd == nullptr)
        return false;

    std::vector<const sn*> primitives;
    nd->collect_prim(primitives);
    int  primitive_count = 0;
    bool outer = false;
    bool inner = false;
    int  complemented = 0;

    for (const sn* primitive : primitives)
    {
        bool sphere_leaf = primitive->typecode == CSG_SPHERE || primitive->typecode == CSG_ZSPHERE;
        if (!sphere_leaf)
            continue;

        const double* param = primitive->getParam();
        if (param == nullptr)
            continue;

        bool z_sphere = primitive->typecode == CSG_ZSPHERE;
        bool quarter_phi = z_sphere && std::fabs(param[0]) < 1.e-9 && std::fabs(param[1] - 0.5 * CLHEP::pi) < 1.e-9;
        bool full_phi =
            primitive->typecode == CSG_SPHERE ||
            (z_sphere && std::fabs(param[0]) < 1.e-9 && std::fabs(param[1]) < 1.e-9);
        bool outer_radius_match = std::fabs(param[3] - outer_radius) < 1.e-9;
        bool inner_radius_match = std::fabs(param[3] - inner_radius) < 1.e-9;

        primitive_count++;
        complemented += primitive->complement == 1 ? 1 : 0;
        outer = outer || (quarter_phi && outer_radius_match && primitive->complement == 0);
        inner = inner || (full_phi && inner_radius_match && primitive->complement == 1);
    }

    return primitive_count == 2 && outer && inner && complemented == 1;
}

int CountPartialPhiSpheres(const G4VSolid* solid)
{
    const G4Sphere* sphere = dynamic_cast<const G4Sphere*>(solid);
    if (sphere)
    {
        double start_phi = sphere->GetStartPhiAngle() / CLHEP::radian;
        double delta_phi = sphere->GetDeltaPhiAngle() / CLHEP::radian;
        bool   partial_phi = start_phi != 0. || delta_phi != 2. * CLHEP::pi;
        return partial_phi ? 1 : 0;
    }

    const G4BooleanSolid* boolean = dynamic_cast<const G4BooleanSolid*>(solid);
    if (boolean == nullptr)
        return 0;

    const G4VSolid* left = boolean->GetConstituentSolid(0);
    const G4VSolid* right = boolean->GetConstituentSolid(1);
    return CountPartialPhiSpheres(left) + CountPartialPhiSpheres(right);
}

ConvertOutcome ConvertInChildProcess(const G4VSolid* solid)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        perror("fork");
        return CONVERT_ERROR;
    }

    if (pid == 0)
    {
        s_csg* csg = new s_csg;
        assert(csg);

        int lvid = 0;
        int depth = 0;
        int level = 1;
        sn* gdml_nd = U4Solid::Convert(solid, lvid, depth, level);

        G4Sphere native_shell("NativeQuarterShell", 95. * CLHEP::mm, 100. * CLHEP::mm, 0., 0.5 * CLHEP::pi, 0., CLHEP::pi);
        sn*      native_nd = U4Solid::Convert(&native_shell, lvid + 1, depth, level);

        bool gdml_ok = HasSinglePhiShell(gdml_nd, 100., 95.);
        bool native_ok = HasSinglePhiShell(native_nd, 100., 95.);
        int  exit_code = gdml_ok && native_ok ? 0 : 5;
        if (exit_code != 0)
        {
            std::cerr << "child conversion did not produce a wedged outer and full inner sphere for both shell forms" << std::endl;
        }
        delete gdml_nd;
        delete native_nd;
        _exit(exit_code);
    }

    int status = 0;
    int rc = waitpid(pid, &status, 0);
    if (rc != pid)
    {
        perror("waitpid");
        return CONVERT_ERROR;
    }

    if (WIFSIGNALED(status))
    {
        int signal = WTERMSIG(status);
        if (IsExpectedRejectSignal(signal))
        {
            std::cout << "child rejected conversion with expected signal " << signal << std::endl;
            return CONVERT_REJECTED;
        }

        std::cerr << "child crashed with unexpected signal " << signal << std::endl;
        return CONVERT_ERROR;
    }

    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
    {
        std::cout
            << "child converted both shell forms with one phi-wedged outer sphere"
            << std::endl;
        return CONVERT_ACCEPTED;
    }

    if (WIFEXITED(status))
    {
        std::cerr << "child exited unexpectedly with status " << WEXITSTATUS(status) << std::endl;
        return CONVERT_ERROR;
    }

    std::cerr << "child ended in unexpected state " << status << std::endl;
    return CONVERT_ERROR;
}
} // namespace

int main(int argc, char** argv)
{
    static plog::ConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::info, &consoleAppender);

    const G4VPhysicalVolume* world = U4GDML::Read(testGeomFile);
    LOG_IF(plog::fatal, world == nullptr)
        << "failed to load GDML path " << (testGeomFile ? testGeomFile : "-");
    if (world == nullptr)
        return EXIT_FAILURE;

    const G4VPhysicalVolume* quarter_shell_pv = U4Volume::FindPV(world, "QuarterShell_pv");
    LOG_IF(plog::fatal, quarter_shell_pv == nullptr)
        << "failed to find QuarterShell_pv in GDML path " << testGeomFile;
    if (quarter_shell_pv == nullptr)
        return EXIT_FAILURE;

    const G4LogicalVolume* quarter_shell_lv = quarter_shell_pv->GetLogicalVolume();
    LOG_IF(plog::fatal, quarter_shell_lv == nullptr)
        << "QuarterShell_pv lacks a logical volume";
    if (quarter_shell_lv == nullptr)
        return EXIT_FAILURE;

    const G4VSolid* quarter_shell_solid = quarter_shell_lv->GetSolid();
    LOG_IF(plog::fatal, quarter_shell_solid == nullptr)
        << "QuarterShell_pv lacks a solid";
    if (quarter_shell_solid == nullptr)
        return EXIT_FAILURE;

    const G4IntersectionSolid* intersection = dynamic_cast<const G4IntersectionSolid*>(quarter_shell_solid);
    LOG_IF(plog::fatal, intersection != nullptr)
        << "test geometry unexpectedly uses a parent IntersectionSolid";
    if (intersection != nullptr)
        return EXIT_FAILURE;

    const G4SubtractionSolid* subtraction = dynamic_cast<const G4SubtractionSolid*>(quarter_shell_solid);
    LOG_IF(plog::fatal, subtraction == nullptr)
        << "test geometry expected a subtraction shell solid " << quarter_shell_solid->GetName();
    if (subtraction == nullptr)
        return EXIT_FAILURE;

    int partial_phi_spheres = CountPartialPhiSpheres(quarter_shell_solid);
    LOG_IF(plog::fatal, partial_phi_spheres == 0)
        << "test geometry expected partial-phi sphere primitives";
    if (partial_phi_spheres == 0)
        return EXIT_FAILURE;

    ConvertOutcome outcome = ConvertInChildProcess(quarter_shell_solid);
    switch (outcome)
    {
    case CONVERT_REJECTED:
        std::cerr
            << "partial-phi sphere conversion was rejected"
            << std::endl;
        return EXIT_FAILURE;

    case CONVERT_ACCEPTED:
        std::cout
            << "partial-phi sphere conversion succeeded with a non-null CSG tree"
            << std::endl;
        return EXIT_SUCCESS;

    case CONVERT_ERROR:
        break;
    }

    std::cerr
        << "SpherePhiCutQuarterShellTest could not confirm partial-phi sphere conversion."
        << std::endl;

    return EXIT_FAILURE;
}
