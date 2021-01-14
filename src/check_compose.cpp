#include "check.h"

#include <iostream>

#include <curl/curl.h>
#include <stdio.h>

#include "inja/nlohmann/json.hpp"

using json = nlohmann::json;

class compose_service {
 public:
  compose_service(std::string name, std::string state, std::string status) : name(name), state(state), status(status) {}
  std::string name;
  std::string state;
  std::string status;
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
  ((std::string *)userp)->append((char *)contents, size * nmemb);
  return size * nmemb;
}

static std::vector<compose_service> compose_services(std::string compose_proj) {
  std::vector<compose_service> services;

  std::string readBuffer;
  std::string paramString;

  struct curl_slist *headers = nullptr;

  CURL *curl = curl_easy_init();
  if (!curl) {
    curl_global_cleanup();
    throw std::runtime_error("Unable to initialize curl");
  }

  headers = curl_slist_append(headers, "Accept: application/json");
  headers = curl_slist_append(headers, "Content-Type: application/json");
  curl_easy_setopt(curl, CURLOPT_UNIX_SOCKET_PATH, "/var/run/docker.sock");

  std::string filters = "{\"label\": [\"com.docker.compose.project=";
  filters += compose_proj;
  filters += "\"], \"status\": [\"created\", \"restarting\", \"running\", \"removing\", \"exited\"]}";
  char *filter_str = curl_easy_escape(curl, filters.c_str(), filters.size());
  std::string url = "http://foo/containers/json?filters=";
  url += filter_str;
  curl_free(filter_str);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    std::string msg = "curl_easy_perform failed: ";
    msg += curl_easy_strerror(res);
    throw std::runtime_error(msg);
  }

  auto j = json::parse(readBuffer);
  for (auto &el : j.items()) {
    auto ctr = el.value();
    services.emplace_back(ctr["Labels"]["com.docker.compose.service"].get<std::string>(),
                          ctr["State"].get<std::string>(), ctr["Status"].get<std::string>());
  }

  return services;
}

ComposeCheck::ComposeCheck(const std::string &service_name) : svc_(service_name) {}

Status ComposeCheck::run() const {
  Status sts{"", StatusVal::OK};
  try {
    for (const auto &svc : compose_services(svc_)) {
      if (sts.summary.size() > 0) {
        sts.summary += ", ";
      }
      sts.summary += svc.name + "(" + svc.state + ")";
      if (svc.state != "running") {
        sts.value = StatusVal::ERROR;
      }
    }
  } catch (const std::exception &ex) {
    sts.value = StatusVal::ERROR;
    sts.summary = ex.what();
  }
  return sts;
}

std::string ComposeCheck::getLog() const { return "TODO"; }
