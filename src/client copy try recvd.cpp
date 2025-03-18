#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iterator>

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <chrono>
#include "Arduino.h"

#include <base64.h> 

WiFiClass WiFi;
ESPClass ESP;
auto start = std::chrono::high_resolution_clock::now();
// 840103373 - stolb1
// 147856157 - stolb2

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

int main(int ac, char* av[]) {
  using namespace painlessmesh;
  try {
    size_t port = 9000;
    std::string ip = "192.168.0.76";
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());
    nodeId = 2;
    std::cout << "station_id = " << nodeId << "\n";

    Scheduler userScheduler;
    boost::asio::io_service io_service;
    mesh.setDebugMsgTypes( ERROR | STARTUP );
    mesh.init(&userScheduler, nodeId);
    mesh.setRoot(false);
    mesh.setContainsRoot(false);
    
    auto pClient = new AsyncClient(io_service);
    painlessmesh::tcp::connect<painlessmesh::Connection, painlessMesh>(
        (*pClient), boost::asio::ip::address::from_string(ip), port, mesh);
        
    mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
      std::cout << "base get msg from " << nodeId << "; length = " << msg.length() << "\n";
    
      // try {
      //   if (msg.find("IMG:") == 0) {
      //     std::string rawData = msg.substr(4);
      //     std::vector<uint8_t> data(rawData.begin(), rawData.end());
      //     cv::Mat frame = cv::imdecode(data, cv::IMREAD_UNCHANGED);
      //     if (!frame.empty()) {
      //       cv::imshow("client", frame);
      //       cv::waitKey(1);
      //     }
      //   }
      // } catch (const std::exception& e) {
      //   std::cerr << "Error decoding image: " << e.what() << '\n';
      // }
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
    // taskSendMessage.enable();
    // taskShowNet.enable();
    
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