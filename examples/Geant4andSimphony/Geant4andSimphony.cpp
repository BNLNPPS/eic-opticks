#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include <string>

#include "G4Cerenkov.hh"
#include "G4Electron.hh"
#include "G4GDMLParser.hh"
#include "G4Material.hh"
#include "G4MaterialPropertiesTable.hh"
#include "G4OpticalPhoton.hh"
#include "G4OpticalPhysics.hh"
#include "G4ParticleGun.hh"
#include "G4Poisson.hh"
#include "G4ProcessManager.hh"
#include "G4ProcessVector.hh"
#include "G4RunManagerFactory.hh"
#include "G4SDManager.hh"
#include "G4Step.hh"
#include "G4SystemOfUnits.hh"
#include "G4Track.hh"
#include "G4UImanager.hh"
#include "G4UserEventAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4VSensitiveDetector.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "Randomize.hh"

#include "FTFP_BERT.hh"

#include "sysrap/OPTICKS_LOG.hh"
#include "sysrap/SEvt.hh"
#include "u4/U4.hh"
#include "g4cx/G4CXOpticks.hh"

namespace {

std::mutex g_opticks_mutex;

class NullSD : public G4VSensitiveDetector {
  public:
    explicit NullSD(const G4String& name) : G4VSensitiveDetector(name) {}
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override { return false; }
};

class GDMLDetector : public G4VUserDetectorConstruction {
  public:
    explicit GDMLDetector(std::string gdml) : gdml_(std::move(gdml)) {}
    G4VPhysicalVolume* Construct() override
    {
        parser_.Read(gdml_, false);
        world_ = parser_.GetWorldVolume();
        return world_;
    }
    void ConstructSDandField() override
    {
        int isd = 0;
        for (const auto& [lv, list] : *parser_.GetAuxMap())
            for (const auto& aux : list)
                if (aux.type == "SensDet") {
                    auto* sd = new NullSD(lv->GetName() + "_sd" + std::to_string(isd++));
                    G4SDManager::GetSDMpointer()->AddNewDetector(sd);
                    lv->SetSensitiveDetector(sd);
                }
        static std::once_flag once;
        if (world_) std::call_once(once, [this] { G4CXOpticks::SetGeometry(world_); });
    }
  private:
    std::string        gdml_;
    G4GDMLParser       parser_;
    G4VPhysicalVolume* world_ = nullptr;
};

class Gun : public G4VUserPrimaryGeneratorAction {
  public:
    Gun(int n, double e_MeV, G4ThreeVector dir) : gun_(n)
    {
        gun_.SetParticleDefinition(G4Electron::Definition());
        gun_.SetParticlePosition({0, 0, 0});
        gun_.SetParticleMomentumDirection(dir);
        gun_.SetParticleEnergy(e_MeV * MeV);
    }
    void GeneratePrimaries(G4Event* evt) override { gun_.GeneratePrimaryVertex(evt); }
  private:
    G4ParticleGun gun_;
};

class CerenkovGenstepAction : public G4UserSteppingAction {
  public:
    void UserSteppingAction(const G4Step* step) override
    {
        const G4Track* track = step->GetTrack();
        const G4ParticleDefinition* def = track->GetDefinition();
        if (def == G4OpticalPhoton::OpticalPhotonDefinition()) return;
        if (def->GetPDGCharge() == 0.0) return;
        const double L = step->GetStepLength();
        if (L <= 0.0) return;
        const G4Material* mat = track->GetMaterial();
        auto* mpt = mat ? mat->GetMaterialPropertiesTable() : nullptr;
        if (!mpt) return;
        auto* rindex = mpt->GetProperty(kRINDEX);
        if (!rindex || rindex->GetVectorLength() == 0) return;

        const double mass = def->GetPDGMass();
        const double T1 = step->GetPreStepPoint()->GetKineticEnergy();
        const double T2 = step->GetPostStepPoint()->GetKineticEnergy();
        const double beta1 = T1 > 0.0 ? std::sqrt(T1 * (T1 + 2.0 * mass)) / (T1 + mass) : 0.0;
        const double beta2 = T2 > 0.0 ? std::sqrt(T2 * (T2 + 2.0 * mass)) / (T2 + mass) : 0.0;
        if (beta1 <= 0.0) return;

        if (!cerenkov_) cerenkov_ = find_cerenkov();
        if (!cerenkov_) return;
        const double charge = def->GetPDGCharge();
        const double mp1 = cerenkov_->GetAverageNumberOfPhotons(charge, beta1, mat, rindex);
        const double mp2 = cerenkov_->GetAverageNumberOfPhotons(charge, std::max(beta2, 1e-3), mat, rindex);
        const double mean = 0.5 * (mp1 + mp2) * L;
        if (mean <= 0.0) return;

        const double betaInv = 2.0 / (beta1 + beta2);
        const double nMax    = rindex->GetMaxValue();
        const double maxCos  = betaInv / nMax;
        const double maxSin2 = (1.0 - maxCos) * (1.0 + maxCos);
        if (maxSin2 <= 0.0) return;
        const G4int n = static_cast<G4int>(G4Poisson(mean));
        if (n <= 0) return;

        constexpr double kCLightMmPerNs = 299.792458;
        const_cast<G4StepPoint*>(step->GetPreStepPoint())->SetVelocity(beta1 * kCLightMmPerNs);
        const_cast<G4StepPoint*>(step->GetPostStepPoint())->SetVelocity(beta2 * kCLightMmPerNs);

        std::lock_guard<std::mutex> lock(g_opticks_mutex);
        U4::CollectGenstep_G4Cerenkov_modified(
            track, step, n, betaInv,
            rindex->Energy(0), rindex->GetMaxEnergy(),
            maxCos, maxSin2, mp1, mp2);
    }
  private:
    static G4Cerenkov* find_cerenkov()
    {
        auto* pm = G4Electron::Definition()->GetProcessManager();
        if (!pm) return nullptr;
        auto* procs = pm->GetProcessList();
        for (G4int i = 0; i < procs->size(); ++i)
            if ((*procs)[i]->GetProcessName() == "Cerenkov")
                return static_cast<G4Cerenkov*>((*procs)[i]);
        return nullptr;
    }
    G4Cerenkov* cerenkov_ = nullptr;
};

class EventAction : public G4UserEventAction {
  public:
    void EndOfEventAction(const G4Event* evt) override
    {
        std::lock_guard<std::mutex> lock(g_opticks_mutex);
        SEvt* sev = SEvt::Get_EGPU();
        const long ngs = (long)sev->getNumGenstepFromGenstep();
        if (ngs <= 0) return;
        G4CXOpticks::Get()->simulate(evt->GetEventID(), false);
        std::printf("[event %d] gensteps=%ld photons=%ld hits=%ld\n",
                    evt->GetEventID(), ngs,
                    (long)SEvt::GetNumPhotonCollected(0),
                    (long)SEvt::GetNumHit(0));
        sev->clear_genstep();
    }
};

class ActionInit : public G4VUserActionInitialization {
  public:
    ActionInit(int ppe, double e_MeV, G4ThreeVector dir) : ppe_(ppe), e_MeV_(e_MeV), dir_(dir) {}
    void Build() const override
    {
        SetUserAction(new Gun(ppe_, e_MeV_, dir_));
        SetUserAction(new EventAction);
        SetUserAction(new CerenkovGenstepAction);
    }
  private:
    int           ppe_;
    double        e_MeV_;
    G4ThreeVector dir_;
};

class PhysicsList : public FTFP_BERT {
  public:
    PhysicsList() : FTFP_BERT(0) { RegisterPhysics(new G4OpticalPhysics); }
};

}

int main(int argc, char** argv)
{
    OPTICKS_LOG(argc, argv);
    const std::string gdml = argc >= 2 ? argv[1] : "lhcb2024_rich1.gdml";
    const bool lhcb = gdml.find("lhcb2024") != std::string::npos;
    if (lhcb) {
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
    const int    nev   = argc >= 3 ? std::atoi(argv[2]) : 1;
    const int    ppe   = std::getenv("PRIMARIES_PER_EVENT") ? std::atoi(std::getenv("PRIMARIES_PER_EVENT")) : 1000;
    const double e_MeV = std::getenv("ELECTRON_MEV") ? std::atof(std::getenv("ELECTRON_MEV")) : (lhcb ? 5000.0 : 10.0);
    const G4ThreeVector dir = lhcb ? G4ThreeVector(0, 0.2, 0.8) : G4ThreeVector(0, 0, 1);
    const long   seed  = std::getenv("RNG_SEED") ? std::atol(std::getenv("RNG_SEED")) : (long)std::time(nullptr);
    const int    nthr  = std::getenv("G4_THREADS") ? std::atoi(std::getenv("G4_THREADS")) : 1;
    CLHEP::HepRandom::setTheSeed(seed);
    if (!std::getenv("QRng__SEED_OFFSET")) {
        const std::string so = std::to_string(seed) + ":0";
        setenv("QRng__SEED_OFFSET", so.c_str(), 1);
    }
    std::printf("[minimal] gdml=%s nev=%d ppe=%d e=%.1fMeV seed=%ld nt=%d\n",
                gdml.c_str(), nev, ppe, e_MeV, seed, nthr);

    SEvt::CreateOrReuse_EGPU();

    auto* rm = G4RunManagerFactory::CreateRunManager(G4RunManagerType::Default);
    rm->SetUserInitialization(new PhysicsList);
    rm->SetUserInitialization(new GDMLDetector(gdml));
    rm->SetUserInitialization(new ActionInit(ppe, e_MeV, dir));

    auto* ui = G4UImanager::GetUIpointer();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "/run/numberOfThreads %d", nthr);
    ui->ApplyCommand(buf);
    ui->ApplyCommand("/run/initialize");
    ui->ApplyCommand("/process/optical/cerenkov/setStackPhotons false");

    rm->BeamOn(nev);
    delete rm;
    return 0;
}
