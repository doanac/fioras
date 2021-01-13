#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

enum class StatusVal {
  OK = 0,
  ERROR,
  UNKNOWN,
};

struct Status {
  std::string summary;
  StatusVal value;

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

std::vector<std::tuple<std::string, std::unique_ptr<CheckInterface>>> checks_list();
