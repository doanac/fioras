#include <iostream>
#include <unordered_map>

#include <boost/program_options.hpp>

#include "check.h"
#include "httpd.h"

namespace bpo = boost::program_options;

static int version_main(const bpo::variables_map& variables_map) {
  std::cout << "Version: " << GIT_COMMIT << "\n";
  return 0;
}

static int checks_main(const bpo::variables_map& variables_map) {
  if (variables_map.count("check-name") != 0) {
    std::string name = variables_map["check-name"].as<std::string>();
    for (const auto& it : checks_list()) {
      if (name == std::get<0>(it)) {
        auto sts = std::get<1>(it)->run();
        std::cout << "Name:  " << name << "\n";
        std::cout << "Status:" << sts.valueString() << " " << sts.summary << "\n";
        std::cout << "Logs:\n";
        std::cout << std::get<1>(it)->getLog();
        return 0;
      }
    }
    std::cerr << "check(" << name << ") not found\n";
    return 1;
  }
  for (const auto& it : checks_list()) {
    std::cout << std::get<0>(it) << ":\t";
    auto sts = std::get<1>(it)->run();
    std::cout << sts.valueString() << "\t";
    std::cout << sts.summary << "\n";
  }
  return 0;
}

static int server_main(const bpo::variables_map& variables_map) {
  int port = variables_map["port"].as<int>();
  return httpd_main(port);
}

static const std::unordered_map<std::string, int (*)(const bpo::variables_map&)> commands = {
    {"version", version_main},
    {"run-checks", checks_main},
    {"run-server", server_main},
};

static void check_info_options(const bpo::options_description& description, const bpo::variables_map& vm) {
  if (vm.count("help") != 0 || vm.count("command") == 0) {
    std::cout << description << '\n';
    exit(EXIT_SUCCESS);
  }
}

static bpo::variables_map parse_options(int argc, char** argv) {
  std::string subs("Command to execute: ");
  for (const auto& cmd : commands) {
    static int indx = 0;
    if (indx != 0) {
      subs += ", ";
    }
    subs += cmd.first;
    ++indx;
  }
  bpo::options_description description("fioras command line options");

  // clang-format off
  description.add_options()
    ("help,h", "print usage")
    ("check-name,c", bpo::value<std::string>(), "Run a specific check.")
    ("port,p", bpo::value<int>()->default_value(80), "Bind httpd server to this port")
    ("command", bpo::value<std::string>(), subs.c_str()
  );
  // clang-format on

  bpo::positional_options_description pos;
  pos.add("command", 1);

  bpo::variables_map vm;
  std::vector<std::string> unregistered_options;
  try {
    bpo::basic_parsed_options<char> parsed_options =
        bpo::command_line_parser(argc, argv).options(description).positional(pos).allow_unregistered().run();
    bpo::store(parsed_options, vm);
    check_info_options(description, vm);
    bpo::notify(vm);
    unregistered_options = bpo::collect_unrecognized(parsed_options.options, bpo::exclude_positional);
    if (vm.count("help") == 0 && !unregistered_options.empty()) {
      std::cout << description << "\n";
      exit(EXIT_FAILURE);
    }
  } catch (const bpo::required_option& ex) {
    std::cout << ex.what() << std::endl << description;
    exit(EXIT_FAILURE);
  } catch (const bpo::error& ex) {
    check_info_options(description, vm);

    // log boost error
    std::cerr << "boost command line option error: " << ex.what();

    // print the error message to the standard output too, as the user provided
    // a non-supported commandline option
    std::cout << ex.what() << '\n';

    // set the returnValue, thereby ctest will recognize
    // that something went wrong
    exit(EXIT_FAILURE);
  }

  return vm;
}

int main(int argc, char** argv) {
  bpo::variables_map commandline_map = parse_options(argc, argv);
  int rc = 0;

  try {
    std::string cmd = commandline_map["command"].as<std::string>();
    auto cmd_to_run = commands.find(cmd);
    if (cmd_to_run == commands.end()) {
      throw bpo::invalid_option_value("Unsupported command: " + cmd);
    }
    rc = (*cmd_to_run).second(commandline_map);
  } catch (const std::exception& ex) {
    std::cerr << ex.what();
    rc = EXIT_FAILURE;
  }

  return rc;
}
