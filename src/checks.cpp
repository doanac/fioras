#include "check.h"

std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks_list() {
  std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks;
  checks.emplace_back("docker", std::make_unique<SystemDCheck>("docker.service"));
  checks.emplace_back("aktualizr-lite", std::make_unique<SystemDCheck>("aktualizr-lite.service"));
  return checks;
}
