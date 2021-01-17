#include <sstream>

#include "cpp-httplib/httplib.h"

#include "check.h"

static void show_checks(httplib::Response &res) {
  std::stringstream ss;
  for (const auto &it : checks_list()) {
    ss << std::get<0>(it) << ":\t";
    auto sts = std::get<1>(it)->run();
    ss << sts.valueString() << "\t";
    ss << sts.summary << "\n";
  }
  res.set_content(ss.str(), "text/plain");
}

static void show_check(const std::string &name, httplib::Response &res) {
  std::stringstream ss;
  for (const auto &it : checks_list()) {
    if (name == std::get<0>(it)) {
      auto sts = std::get<1>(it)->run();
      ss << "Name:  " << name << "\n";
      ss << "Status:" << sts.valueString() << " " << sts.summary << "\n";
      ss << "Logs:\n";
      ss << std::get<1>(it)->getLog();
      res.set_content(ss.str() + "\n", "text/plain");
      return;
    }
  }
  ss << "check(" << name << ") not found\n";
  res.status = 404;
  res.reason = "Not Found";
  res.set_content(ss.str() + "\n", "text/plain");
}

int httpd_main(int port) {
  httplib::Server svr;

  if (!svr.is_valid()) {
    std::cerr << "Error constructing server\n";
    return 1;
  }

  svr.Get("/", [](const httplib::Request &, httplib::Response &res) { show_checks(res); });

  svr.Get("/checks/(.*)", [](const httplib::Request &req, httplib::Response &res) { show_check(req.matches[1], res); });

  svr.set_logger([](const httplib::Request &req, const httplib::Response &resp) {
    std::cerr << req.method << " " << req.path << " HTTP_" << resp.status << " " << resp.reason << "\n";
  });

  return !svr.listen("0.0.0.0", port);
}
