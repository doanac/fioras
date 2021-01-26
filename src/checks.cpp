#include "check.h"

#include "boost/filesystem.hpp"

std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks_list() {
  std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks;
  checks.emplace_back("docker", std::make_unique<SystemDCheck>("docker.service"));
  checks.emplace_back("aktualizr-lite", std::make_unique<SystemDCheck>("aktualizr-lite.service"));

  boost::filesystem::path p("/var/sota/compose-apps");
  for (const auto &it : boost::filesystem::directory_iterator(p)) {
    if (boost::filesystem::is_directory(it.path())) {
      auto app = it.path().filename().native();
      checks.emplace_back(app, std::make_unique<ComposeCheck>(app));
    }
  }
  return checks;
}
