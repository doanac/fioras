#include "check.h"

#include <openssl/asn1.h>
#include <openssl/bio.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <string.h>
#include <fstream>

// Smart pointers to wrap openssl C types that need explicit free
using BIO_ptr = std::unique_ptr<BIO, decltype(&BIO_free)>;
using X509_ptr = std::unique_ptr<X509, decltype(&X509_free)>;

static std::string _device_uuid() {
  BIO_ptr input(BIO_new(BIO_s_file()), BIO_free);
  if (BIO_read_filename(input.get(), "/var/sota/client.pem") <= 0) {
    return "???";
  }
  X509_ptr cert(PEM_read_bio_X509_AUX(input.get(), NULL, NULL, NULL), X509_free);
  BIO_ptr subj_bio(BIO_new(BIO_s_mem()), BIO_free);
  X509_NAME *subject = X509_get_subject_name(cert.get());

  int common_name_loc = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
  if (common_name_loc < 0) {
    return "???";
  }

  X509_NAME_ENTRY *common_name_entry = X509_NAME_get_entry(subject, common_name_loc);
  if (common_name_entry == NULL) {
    return "???";
  }
  ASN1_STRING *common_name_asn1 = X509_NAME_ENTRY_get_data(common_name_entry);
  if (common_name_asn1 == NULL) {
    return "???";
  }
  return (const char *)ASN1_STRING_data(common_name_asn1);
}

SysInfo GetSysInfo() {
  SysInfo info{};

  info.device_uuid = _device_uuid();

  std::ifstream file("/var/sota/current-target");
  if (file.fail()) {
    return info;
  }
  std::string line;
  while (std::getline(file, line)) {
    if (line.find("TARGET_NAME=") == 0) {
      auto start = line.find('"') + 1;
      auto finish = line.rfind('"');
      info.target_name = line.substr(start, finish - start);
    } else if (line.find("CUSTOM_VERSION=") == 0) {
      auto start = line.find('"') + 1;
      auto finish = line.rfind('"');
      info.target_version = line.substr(start, finish - start);
    }
  }

  return info;
}