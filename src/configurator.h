#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace xpload {

struct DbConfig
{
  std::string host{"localhost"};
  std::string port{"8080"};
  std::string apiroot{"/api/cdb_rest"};
  std::string apiver{""};
  std::vector<std::filesystem::path> path{"/path/to/payload/data"};
  bool use_cache{false};
  bool dry_run{false};
  int verbosity{0};
  int retry_times{5};
  int retry_max_delay{60};

  std::string url() const { return "http://" + host + ':' + port + apiroot; }
};


/**
 * Provides access to all configuration types and data.
 */
class Configurator
{
 public:

  Configurator(std::string config_name = "prod");

  /// A unique name associated with this Configurator
  std::string name;

  DbConfig db;

 private:

  std::string Locate(std::string filename) const;
  void ReadConfig(std::string filepath);
};

}
