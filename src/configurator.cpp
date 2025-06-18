#include <cstdlib>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <vector>

#include <nlohmann/json.hpp>

#include "xpload/configurator.h"
#include "config.h"

namespace xpload {


Configurator::Configurator(std::string config_name) :
  name{std::getenv("XPLOAD_CONFIG") ? std::getenv("XPLOAD_CONFIG") : config_name}
{
  ReadConfig(Locate(name + ".json"));
}


std::string Configurator::Locate(std::string filename) const
{
  std::vector<std::string> search_paths;

  const std::string user_dir{std::getenv("XPLOAD_CONFIG_DIR") ? std::getenv("XPLOAD_CONFIG_DIR") : ""};

  if (user_dir.empty())
  {
    std::string paths(XPLOAD_CONFIG_SEARCH_PATHS);

    size_t last = 0;
    size_t next = 0;
    while ((next = paths.find(':', last)) != std::string::npos)
    {
      search_paths.push_back(paths.substr(last, next-last));
      last = next + 1;
    }

    search_paths.push_back(paths.substr(last));
  }
  else
  {
    search_paths.push_back(user_dir);
  }

  struct stat buffer;
  std::string filepath{""};
  for (std::string path : search_paths)
  {
    std::string fpath{path + "/" + filename};
    if (stat(fpath.c_str(), &buffer) == 0)
    {
      filepath = fpath;
      break;
    }
  }

  if (filepath.empty())
  {
    std::string errmsg{"Could not find config file \"" + filename + "\" in "};
    for (std::string path : search_paths) errmsg += (path + ":");
    throw std::runtime_error(errmsg);
  }

  return filepath;
}


/**
 * Expects a valid filepath.
 */
void Configurator::ReadConfig(std::string filepath)
{
  nlohmann::json json;

  try {
    std::ifstream ifs(filepath);
    ifs >> json;

    db = {
      json["host"], json["port"],
      json["apiroot"], json["apiver"],
      json["path"],
      json["use_cache"],
      json["dry_run"],
      json["verbosity"],
      json["retry_times"],
      json["retry_max_delay"]
    };
  }
  catch (nlohmann::json::exception& e) {
    std::string errmsg{"Failed reading config parameters from " + filepath + "\n" + e.what()};
    throw std::runtime_error{errmsg};
  }

  if (db.retry_times < 0)   db.retry_times = 0;
  if (db.retry_times > 100) db.retry_times = 100;

  if (db.retry_max_delay < 1)   db.retry_max_delay = 1;
  if (db.retry_max_delay > 100) db.retry_max_delay = 100;
}

}
