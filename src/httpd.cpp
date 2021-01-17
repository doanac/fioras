#include "cpp-httplib/httplib.h"

static void show_checks(httplib::Response &res) { res.set_content("Hello World!\n", "text/plain"); }

static void show_check(const std::string &name, httplib::Response &res) { res.set_content(name + "\n", "text/plain"); }

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
