#include "check.h"

#include <netinet/in.h>
#include <iostream>
#include <sstream>

#include <curl/curl.h>
#include <stdio.h>

#include "inja/nlohmann/json.hpp"

using json = nlohmann::json;

class compose_service {
 public:
  compose_service(std::string container_id, std::string name, std::string state, std::string status,
                  std::vector<Link> links)
      : container_id(container_id), name(name), state(state), status(status), links(links) {}
  std::string container_id;
  std::string name;
  std::string state;
  std::string status;
  std::vector<Link> links;
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
  filters += compose_proj + "\"]}";
  char *filter_str = curl_easy_escape(curl, filters.c_str(), filters.size());
  std::string url = "http://foo/containers/json?all=1&filters=";
  url += filter_str;
  curl_free(filter_str);

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    std::string msg = "curl_easy_perform failed: ";
    msg += curl_easy_strerror(res);
    throw std::runtime_error(msg);
  }

  auto j = json::parse(readBuffer);
  for (auto &el : j.items()) {
    auto ctr = el.value();
    std::string svc = ctr["Labels"]["com.docker.compose.service"].get<std::string>();

    std::vector<Link> links;
    for (const auto &port : ctr["Ports"]) {
      std::string lbl = compose_proj + "-" + svc;
      if (port.contains("PublicPort")) {
        links.emplace_back(lbl, port["IP"].get<std::string>(), port["PublicPort"].get<uint16_t>());
      }
    }

    services.emplace_back(ctr["Id"], svc, ctr["State"].get<std::string>(), ctr["Status"].get<std::string>(), links);
  }

  return services;
}

static std::string container_logs(std::string container_id) {
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

  std::string url = "http://foo/containers/";
  url += container_id + "/logs?stderr=1&stdout=1&tail=20";

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    std::string msg = "curl_easy_perform failed: ";
    msg += curl_easy_strerror(res);
    throw std::runtime_error(msg);
  }
  return readBuffer;
}

static bool container_has_tty(std::string container_id) {
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

  std::string url = "http://foo/containers/";
  url += container_id + "/json";

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

  CURLcode res = curl_easy_perform(curl);
  curl_easy_cleanup(curl);
  if (res != CURLE_OK) {
    std::string msg = "curl_easy_perform failed: ";
    msg += curl_easy_strerror(res);
    throw std::runtime_error(msg);
  }

  auto j = json::parse(readBuffer);
  return j["Config"]["Tty"].get<bool>();
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

      sts.links.insert(sts.links.end(), svc.links.begin(), svc.links.end());
    }
  } catch (const std::exception &ex) {
    sts.value = StatusVal::ERROR;
    sts.summary = ex.what();
  }
  return sts;
}

std::string ComposeCheck::getLog() const {
  std::stringstream ss;
  try {
    for (const auto &svc : compose_services(svc_)) {
      ss << "## " << svc.name << " --------------------\n";
      std::string buf = container_logs(svc.container_id);
      if (container_has_tty(svc.container_id)) {
        ss << buf;
      } else {
        //  buf =  [8]byte{STREAM_TYPE, 0, 0, 0, SIZE1, SIZE2, SIZE3, SIZE4}[]byte{OUTPUT}
        // ignore first 4 bytes, its just saying stderr/stdout - we don't distinguish
        size_t idx = 0;
        while (idx < buf.size()) {
          int len = ntohl(*(uint32_t *)(buf.c_str() + idx + 4));
          ss << std::string(buf.c_str() + idx + 8, len);
          idx += 8 + len;
        }
      }
    }
  } catch (const std::exception &ex) {
    ss << "\nERROR reading log: " << ex.what() << "\n";
  }
  return ss.str();
}
