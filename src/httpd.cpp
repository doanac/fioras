#include <sstream>

#include "cpp-httplib/httplib.h"
#include "inja/inja.hpp"
#include "inja/nlohmann/json.hpp"

#include "check.h"

using json = nlohmann::json;

const std::string detailsTmpl = R"~~~~(
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <title>Fiodash</title>
    <link rel="stylesheet" href="https://cdn.jsdelivr.net/npm/bulma@0.9.1/css/bulma.min.css">
  </head>
  <body>
  <section class="section">
    <div class="container">
      <h1 class="title">
        Check for {{ name }}
      </h1>
      <table class="table">
        <tr>
          <th>Status</th>
          <td><span {{ val_style }}>{{ val }}</span></td>
        </tr>
        <tr>
          <th>Summary</th>
          <td>{{ summary }}</td>
        </tr>
      </table>
      <pre>{{ log }}</pre>
    </div>
  </section>
  </body>
</html>
)~~~~";

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
  for (const auto &it : checks_list()) {
    if (name == std::get<0>(it)) {
      auto sts = std::get<1>(it)->run();
      json data;
      data["name"] = name;
      data["val"] = sts.valueString();
      data["summary"] = sts.summary;
      data["log"] = std::get<1>(it)->getLog();
      if (sts.value != StatusVal::OK) {
        data["val_style"] = "class=\"is-danger\"";
      } else {
        data["val_style"] = "";
      }
      try {
        res.set_content(inja::render(detailsTmpl, data), "text/html");
      } catch (const std::exception &ex) {
        res.status = 500;
        res.set_content(ex.what(), "text/plain");
      }
      return;
    }
  }
  std::stringstream ss;
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

  svr.set_error_handler([](const httplib::Request & /*req*/, httplib::Response &res) {
    if (res.body.size() == 0) {
      const char *fmt = "<p>Error Status: <span style='color:red;'>%d</span></p>";
      char buf[BUFSIZ];
      snprintf(buf, sizeof(buf), fmt, res.status);
      res.set_content(buf, "text/html");
    }
  });

  return !svr.listen("0.0.0.0", port);
}
