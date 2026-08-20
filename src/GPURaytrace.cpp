#include <cstdlib>
#include <string>

#include <argparse/argparse.hpp>

#include "FTFP_BERT.hh"
#include "G4OpticalPhysics.hh"
#include "G4VModularPhysicsList.hh"

#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4VisExecutive.hh"

#include "sysrap/OPTICKS_LOG.hh"

#include "GPURaytrace.h"
#include "config.h"

#include "G4RunManager.hh"
#include "G4RunManagerFactory.hh"
#include "G4VUserActionInitialization.hh"

using namespace std;

struct ActionInitialization : public G4VUserActionInitialization
{
  private:
    G4App *fG4App; // Store the pointer to G4App

  public:
    // Note the signature: now we take a pointer to the G4App itself
    ActionInitialization(G4App *app) : G4VUserActionInitialization(), fG4App(app)
    {
    }

    virtual void BuildForMaster() const override
    {
        SetUserAction(fG4App->run_act_);
    }

    virtual void Build() const override
    {
        SetUserAction(fG4App->prim_gen_);
        SetUserAction(fG4App->run_act_);
        SetUserAction(fG4App->event_act_);
        SetUserAction(fG4App->tracking_);
        SetUserAction(fG4App->stepping_);
    }
};

int main(int argc, char **argv)
{

    OPTICKS_LOG(argc, argv);

    argparse::ArgumentParser program("GPURaytrace", "0.0.0");

    string gdml_file, config_name, macro_name;
    bool interactive;

    program.add_argument("-g", "--gdml")
        .help("path to GDML file")
        .default_value(string("geom.gdml"))
        .nargs(1)
        .store_into(gdml_file);

    program.add_argument("-c", "--config")
        .help("the name of a config file")
        .default_value(string("dev"))
        .nargs(1)
        .store_into(config_name);

    program.add_argument("-m", "--macro")
        .help("path to G4 macro")
        .default_value(string("run.mac"))
        .nargs(1)
        .store_into(macro_name);

    program.add_argument("-i", "--interactive")
        .help("whether to open an interactive window with a viewer")
        .flag()
        .store_into(interactive);

    program.add_argument("-s", "--seed").help("fixed random seed (default: time-based)").scan<'i', long>();

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const exception &err)
    {
        cerr << err.what() << endl;
        cerr << program;
        exit(EXIT_FAILURE);
    }

    long seed;
    if (program.is_used("--seed"))
    {
        seed = program.get<long>("--seed");
    }
    else
    {
        seed = static_cast<long>(time(nullptr));
    }
    CLHEP::HepRandom::setTheSeed(seed);
    G4cout << "Random seed set to: " << seed << G4endl;

    if (gdml_file.find("lhcb2024") != string::npos)
    {
        setenv("U4Tree__DEDUP_SURFACES", "1", 0);
        setenv("U4Solid__PERMISSIVE", "1", 0);
        setenv("U4Polycone__ENABLE_PHICUT", "1", 0);
        setenv("U4Solid__PRUNE_HUGE_CLIP_BOXES_LV", "Mirror1QuModule", 0);
        setenv("U4Solid__SPHERE_INNER_GROW_Z_MM", "1", 0);
        setenv("U4Mesh__BOX_PLACEHOLDER_BOOLEAN", "1", 0);
        setenv("stree__is_auto_triangulate_NAMES", "notsupported", 0);
        setenv("OPTICKS_CATHODE_M2", "8", 0);
        setenv("OPTICKS_CATHODE_EXIT_MM", "0.3", 0);
    }

    if (!getenv("QRng__SEED_OFFSET"))
    {
        string seed_offset = to_string(seed) + ":0";
        setenv("QRng__SEED_OFFSET", seed_offset.c_str(), 1);
        G4cout << "Opticks photon RNG QRng__SEED_OFFSET set to: " << seed_offset << G4endl;
    }

    simphony::Config{config_name};

    // Configure Geant4
    // The physics list must be instantiated before other user actions
    G4VModularPhysicsList *physics = new FTFP_BERT;
    physics->RegisterPhysics(new G4OpticalPhysics);

    auto *run_mgr = G4RunManagerFactory::CreateRunManager();
    run_mgr->SetUserInitialization(physics);

    G4App *g4app = new G4App(gdml_file);

    ActionInitialization *actionInit = new ActionInitialization(g4app);
    run_mgr->SetUserInitialization(actionInit);
    run_mgr->SetUserInitialization(g4app->det_cons_);

    G4UIExecutive *uix = nullptr;
    G4VisManager *vis = nullptr;

    if (interactive)
    {
        uix = new G4UIExecutive(argc, argv);
        vis = new G4VisExecutive;
        vis->Initialize();
    }

    G4UImanager *ui = G4UImanager::GetUIpointer();
    ui->ApplyCommand("/control/execute " + macro_name);

    if (interactive)
    {
        uix->SessionStart();
    }

    delete uix;

    return EXIT_SUCCESS;
}
