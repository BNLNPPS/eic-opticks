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
class Config
{
 public:

  Config(std::string config_name = "dev");

  /// A unique name associated with this Config
  std::string name;

  storch torch;

 private:

  std::string Locate(std::string filename) const;
  void ReadConfig(std::string filepath);
};

}
