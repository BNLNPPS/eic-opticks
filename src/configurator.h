#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "sysrap/srng.h"
#include "sysrap/storch.h"

namespace gphox {


/**
 * Provides access to all configuration types and data.
 */
class Configurator
{
 public:

  Configurator(std::string config_name = "dev");

  /// A unique name associated with this Configurator
  std::string name;

  storch torch;

 private:

  std::string Locate(std::string filename) const;
  void ReadConfig(std::string filepath);
};

}
