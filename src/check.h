#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

struct SysInfo {
  std::string target_name;
  std::string target_version;
  std::string device_uuid;
};

SysInfo GetSysInfo();

struct Link {
  Link(const std::string label, const std::string ip, const uint16_t port) : label(label), ip(ip), port(port) {}

  std::string label;
  std::string ip;
  uint16_t port;

  std::string getUrl(const std::string host_addr) const {
    if (ip == "0.0.0.0" || ip == host_addr) {
      return "http://" + host_addr + ":" + std::to_string(port);
    }

    if (ip == "127.0.0.1" || ip == "127.0.1.1") {
      if (host_addr == ip) {
        return "http://" + ip + ":" + std::to_string(port);
      }

      // TODO we could probably try and reverse proxy this
      return "";
    }

    // Not sure if we can access this link, but return it anyway
    return "http://" + ip + ":" + std::to_string(port);
  }
};

enum class StatusVal {
  OK = 0,
  ERROR,
  UNKNOWN,
};

struct Status {
  std::string summary;
  StatusVal value;
  std::vector<Link> links;

  std::string valueString() {
    if (value == StatusVal::OK) {
      return "OK";
    }
    if (value == StatusVal::ERROR) {
      return "ERROR";
    }
    if (value == StatusVal::UNKNOWN) {
      return "UNKNOWN";
    }
    throw std::runtime_error("Unknown status value");
  }
};

class CheckInterface {
 public:
  virtual ~CheckInterface() = default;
  virtual std::string getLog() const = 0;
  virtual Status run() const = 0;
};

class SystemDCheck : public CheckInterface {
 public:
  SystemDCheck(const std::string& service_name);
  std::string getLog() const override;
  Status run() const override;

 private:
  std::string svc_;
};

class ComposeCheck : public CheckInterface {
 public:
  ComposeCheck(const std::string& service_name);
  std::string getLog() const override;
  Status run() const override;

 private:
  std::string svc_;
};

class AkliteCheck : public CheckInterface {
 public:
  std::string getLog() const override;
  Status run() const override;
};

std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks_list();