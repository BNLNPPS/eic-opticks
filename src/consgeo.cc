#include <algorithm>
#include <iostream>
#include <string>

#include "sysrap/OPTICKS_LOG.hh"

#include <argparse/argparse.hpp>

#include "g4cx/G4CXOpticks.hh"

using namespace std;

int main(int argc, char **argv)
{
    OPTICKS_LOG(argc, argv);

    argparse::ArgumentParser program("consgeo", "0.0.0");

    string gdml_file;
    string out_prefix;

    program.add_argument("-g", "--gdml")
        .help("path to GDML file")
        .default_value(string("geom.gdml"))
        .nargs(1)
        .store_into(gdml_file);

    program.add_argument("-o", "--out-prefix")
        .help("where to save CSG")
        .default_value(string("csg"))
        .nargs(1)
        .store_into(out_prefix);

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

    LOG_INFO << "gdml_file: " << gdml_file << endl;

    from_gdml(gdml_file, out_prefix);

    return EXIT_SUCCESS;
}
