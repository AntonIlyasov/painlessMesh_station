#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iterator>

#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

#include "painlessmesh/protocol.hpp"
#include "painlessmesh/mesh.hpp"

// TODO Should not be needed anymore in versions after 1.4.9 
using namespace painlessmesh;

#include "painlessMesh.h"
#include "painlessmesh/connection.hpp"
#include "plugin/performance.hpp"


painlessmesh::logger::LogClass Log;

#undef F
#include <boost/date_time.hpp>
#include <boost/program_options.hpp>
#define F(string_literal) string_literal
namespace po = boost::program_options;

#include <iostream>
#include <iterator>
#include <limits>
#include <random>

#define OTA_PART_SIZE 1024
#include "ota.hpp"

template <class T>
// bool contains(T &v, T::value_type const value) {
bool contains(T& v, std::string const value) {
  return std::find(v.begin(), v.end(), value) != v.end();
}

std::string timeToString() {
  boost::posix_time::ptime timeLocal =
      boost::posix_time::second_clock::local_time();
  return to_iso_extended_string(timeLocal);
}

// Will be used to obtain a seed for the random number engine
static std::random_device rd;
static std::mt19937 gen(rd());

uint32_t runif(uint32_t from, uint32_t to) {
  std::uniform_int_distribution<uint32_t> distribution(from, to);
  return distribution(gen);
}

int main(int ac, char* av[]) {
  using namespace painlessmesh;
  try {
    size_t port = 5555;
    std::string ip = "";
    std::vector<std::string> logLevel;
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());
    nodeId = 11111;
    std::cout << "station_id = " << nodeId << "\n";

    Scheduler scheduler;
    boost::asio::io_service io_service;
    painlessMesh mesh;
    Log.setLogLevel(ERROR);
    mesh.init(&scheduler, nodeId);
    std::shared_ptr<AsyncServer> pServer;   

    pServer = std::make_shared<AsyncServer>(io_service, port);
    painlessmesh::tcp::initServer<painlessmesh::Connection, painlessMesh>(*pServer, mesh);

    if (logLevel.size() == 0 || contains(logLevel, "receive")) {
      mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
        std::cout << "{\"event\":\"receive\",\"nodeTime\":"
                  << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                  << "\""
                  << ",\"nodeId\":" << nodeId << ",\"msg\":\"" << msg << "\"}"
                  << std::endl;
      });
    }
    if (logLevel.size() == 0 || contains(logLevel, "connect")) {
      mesh.onNewConnection([&mesh](uint32_t nodeId) {
        std::cout << "{\"event\":\"connect\",\"nodeTime\":"
                  << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                  << "\""
                  << ",\"nodeId\":" << nodeId
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });
    }
    if (logLevel.size() == 0 || contains(logLevel, "disconnect")) {
      mesh.onDroppedConnection([&mesh](uint32_t nodeId) {
        std::cout << "{\"event\":\"disconnect\",\"nodeTime\":"
                  << mesh.getNodeTime() << ",\"time\":\"" << timeToString()
                  << "\""
                  << ",\"nodeId\":" << nodeId
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });
    }
    if (logLevel.size() == 0 || contains(logLevel, "change")) {
      mesh.onChangedConnections([&mesh]() {
        std::cout << "{\"event\":\"change\",\"nodeTime\":" << mesh.getNodeTime()
                  << ",\"time\":\"" << timeToString() << "\""
                  << ", \"layout\":" << mesh.asNodeTree().toString() << "}"
                  << std::endl;
      });
    }
    if (logLevel.size() == 0 || contains(logLevel, "offset")) {
      mesh.onNodeTimeAdjusted([&mesh](int32_t offset) {
        std::cout << "{\"event\":\"offset\",\"nodeTime\":" << mesh.getNodeTime()
                  << ",\"time\":\"" << timeToString() << "\""
                  << ",\"offset\":" << offset << "}" << std::endl;
      });
    }
    if (logLevel.size() == 0 || contains(logLevel, "delay")) {
      mesh.onNodeDelayReceived([&mesh](uint32_t nodeId, int32_t delay) {
        std::cout << "{\"event\":\"delay\",\"nodeTime\":" << mesh.getNodeTime()
                  << ",\"time\":\"" << timeToString() << "\""
                  << ",\"nodeId\":" << nodeId << ",\"delay\":" << delay << "}"
                  << std::endl;
      });
    }
    while (true) {
      usleep(1000);  // Tweak this for acceptable cpu usage
      mesh.update();
      io_service.poll();
    }
  } catch (std::exception& e) {
    std::cerr << "error: " << e.what() << std::endl;
    ;
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!" << std::endl;
    ;
  }
    


  std::cout << "hello\n";
  return 0;
}