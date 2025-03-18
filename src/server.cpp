#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iterator>
#include <chrono>
#include "Arduino.h"

// Глобальные переменные для измерения скорости
size_t totalBytesSent = 0;
std::chrono::steady_clock::time_point startTime;

WiFiClass WiFi;
ESPClass ESP;

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
int val = 0;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void showNet();

Task taskSendMessage( TASK_SECOND * 1 , TASK_FOREVER, &sendMessage );
Task taskShowNet( TASK_SECOND * 10 , TASK_FOREVER, &showNet );

void showNet(){
}

std::string generateRandomString(size_t size) {
  std::random_device rd;  // Источник случайных чисел
  std::mt19937 gen(rd()); // Генератор случайных чисел
  std::uniform_int_distribution<> dis(1, 127); // Диапазон ASCII символов

  std::string result;
  result.reserve(size); // Резервируем память для строки

  for (size_t i = 0; i < size; ++i) {
      result.push_back(static_cast<char>(dis(gen))); // Добавляем случайный символ
  }

  return result;
}

void sendMessage() {

  size_t size = 1024*1024;
  std::string msg = generateRandomString(size);

  std::cout << "send broadcast msg.size() = " << msg.size() << "\n";

  size_t totalSize = msg.size();

  auto start = std::chrono::high_resolution_clock::now();
  if(mesh.sendSingle(2, msg)){
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = end - start;
    std::cout << "Время выполнения !5!: " << duration.count() << " секунд." << std::endl;
  }
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
    size_t nodeId = runif(0, std::numeric_limits<uint32_t>::max());
    nodeId = 1;
    std::cout << "station_id = " << nodeId << "\n";

    Scheduler userScheduler;
    boost::asio::io_service io_service;
    mesh.setDebugMsgTypes( ERROR | STARTUP );
    mesh.init(&userScheduler, nodeId);
    mesh.setRoot(true);
    mesh.setContainsRoot(true);
    
    std::shared_ptr<AsyncServer> pServer = std::make_shared<AsyncServer>(io_service, port);
    painlessmesh::tcp::initServer<painlessmesh::Connection, painlessMesh>(*pServer, mesh);
        
    mesh.onReceive([&mesh](uint32_t nodeId, std::string& msg) {
      std::cout << "base get msg from " << nodeId << " msg: " << msg << "\n";
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