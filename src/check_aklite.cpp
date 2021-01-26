
#include "check.h"

#include "boost/filesystem.hpp"

Status AkliteCheck::run() const {
  if (!boost::filesystem::is_regular_file("/var/sota/client.pem")) {
    auto sts = SystemDCheck("lmp-device-auto-register.service").run();
    sts.summary = "Device not registered. lmp-device-auto-register " + sts.summary;
    return sts;
  }

  return SystemDCheck("aktualizr-lite.service").run();
}

std::string AkliteCheck::getLog() const {
  if (!boost::filesystem::is_regular_file("/var/sota/client.pem")) {
    return SystemDCheck("lmp-device-autoregister.service").getLog();
  }
  return SystemDCheck("aktualizr-lite.service").getLog();
}