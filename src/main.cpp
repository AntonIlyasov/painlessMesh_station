#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iterator>
#include <cpr/cpr.h>
#include <chrono>
#include "json.hpp"

#include "Arduino.h"

WiFiClass WiFi;
ESPClass ESP;

// 840103373 - stolb1
// 147856157 - stolb2

#include "painlessmesh/protocol.hpp"
#include "painlessmesh/mesh.hpp"

// TODO Should not be needed anymore in versions after 1.4.9 
using namespace painlessmesh;
using namespace nlohmann;

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

painlessMesh mesh;
int val;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void showNet();

Task taskSendMessage( TASK_SECOND * 5 , TASK_FOREVER, &sendMessage );
Task taskShowNet( TASK_SECOND * 10 , TASK_FOREVER, &showNet );

void showNet(){
}

void sendMessage() {
  if (val == 0){
    val = 1;
  }
  else {
    val = 0;
  }
  std::string msg = std::to_string(val);
  std::cout << "send broadcast msg = " << msg << "\n";
  mesh.sendBroadcast( msg );
}

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

bool post_event(uint8_t id, uint8_t state, uint8_t status) {

  std::chrono::high_resolution_clock::time_point current_unix_time = std::chrono::high_resolution_clock::now();
  std::time_t t_c = std::chrono::system_clock::to_time_t(current_unix_time);
  std::string timestring = std::ctime(&t_c);
  
  json data = { {"time", timestring}, 
                {"data",{
                  {"id", id},
                  {"state", state},
                  {"status",status}
                }}
              };

  cpr::Response response =  cpr::Post(cpr::Url("http://185.241.68.155:8801/send_data"/*"http://www.httpbin.org/post"*/), 
                            cpr::Body(data.dump()),
                            cpr::Header{{"Content-Type", "application/json"}});

  if (response.status_code == 200) {
    return true;
  } else {
    std::cout << "\nresponse code ERROR: " << response.status_code  << std::endl;
    std::cout << timestring  << std::endl;
    std::cout << response.text  << std::endl;
  }
  return false;
}

int main(int ac, char* av[]) {
  using namespace painlessmesh;
  try {
    size_t port = 5555;
    std::string ip = "10.27.29.1";
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());
    nodeId = 11111;
    std::cout << "station_id = " << nodeId << "\n";

    Scheduler userScheduler;
    boost::asio::io_service io_service;
    mesh.setDebugMsgTypes( ERROR | STARTUP );
    mesh.init(&userScheduler, nodeId);
    mesh.setRoot(true);
    mesh.setContainsRoot(true);
    std::shared_ptr<AsyncServer> pServer;
    
    pServer = std::make_shared<AsyncServer>(io_service, port);
    painlessmesh::tcp::initServer<painlessmesh::Connection, painlessMesh>(*pServer, mesh);
    
    auto pClient = new AsyncClient(io_service);
    painlessmesh::tcp::connect<painlessmesh::Connection, painlessMesh>(
        (*pClient), boost::asio::ip::address::from_string(ip), port, mesh);
    std::cout << mesh.sendBroadcast("test_msg") << "\n";

    mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
      std::cout << "base get msg from " << nodeId << " " << msg << "\n";
    });

    mesh.onNewConnection([&mesh](uint32_t nodeId) {
      std::cout << "New Connection, nodeId = " << nodeId << "\n";
    });

    mesh.onChangedConnections([&mesh]() {
      std::cout << "Changed connections\n" << mesh.subConnectionJson(true).c_str() << "\n";
    });

    mesh.onNodeTimeAdjusted([&mesh](int32_t offset) {
      std::cout << "Adjusted time " << mesh.getNodeTime() << ". Offset = " << offset << "\n";
    });


    userScheduler.addTask( taskSendMessage );
    taskSendMessage.enable();
    taskShowNet.enable();
    
    /*======================POST========================*/
    /*======================POST========================*/
    if (!post_event(11,22,33)) exit(0);
    /*======================POST========================*/
    /*======================POST========================*/
    while (true) {
      mesh.update();
      io_service.poll();
    }
  } catch (std::exception& e) {
    std::cerr << "error1: " << e.what() << std::endl;
    ;
    return 1;
  } catch (...) {
    std::cerr << "Exception of unknown type!" << std::endl;
    ;
  }
  
  std::cout << "END OF THE PROGRAM\n";
  return 0;
}