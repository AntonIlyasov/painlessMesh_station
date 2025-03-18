#include <algorithm>
#include <boost/algorithm/hex.hpp>
#include <boost/filesystem.hpp>
#include <iostream>
#include <iterator>

#include "Arduino.h"

#include <opencv2/opencv.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>

#include <base64.h> 

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

cv::VideoCapture choosen_cap;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
void showNet();

Task taskSendMessage( TASK_MILLISECOND * 43 , TASK_FOREVER, &sendMessage );
Task taskShowNet( TASK_SECOND * 10 , TASK_FOREVER, &showNet );

void showNet(){
}


void encodeAndSendImage(const cv::Mat &frame) {
  if (frame.empty()) {
    std::cerr << "Frame is empty!\n";
    return;
  }

  std::vector<uint8_t> imgImencode;
  if (!cv::imencode(".jpg", frame, imgImencode, std::vector<int> {cv::IMWRITE_JPEG_QUALITY, 100})) {
    std::cerr << "Failed to encode image!\n";
    return;
  }

  std::string msg = "IMG:";
  msg.resize(msg.size() + imgImencode.size());
  std::copy(imgImencode.begin(), imgImencode.end(), msg.begin() + 4);

  for (size_t i = 0; i < 20; i++)
  {
    char c = msg[i];
    printf("ASCII code of '%c' is: %d\n", c, static_cast<int>(c));
  }
  std::cout << std::endl;

  std::cout << "Sending message, msg.size(): " << msg.size() << " bytes\n";
  if (!mesh.sendSingle(2, msg)) {
    std::cerr << "Failed to send message!\n";
  }
}

void sendMessage() {
  cv::Mat frame;
  int ret = choosen_cap.read(frame) ? 1 : 0;
  if (ret) {
    // cv::imshow("server", frame);
    encodeAndSendImage(frame);
    cv::waitKey(1);
  }
  val++;
  // std::string msg = std::to_string(val);
  // std::cout << "send broadcast msg = " << msg << "\n";
  // mesh.sendBroadcast( msg );
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

  choosen_cap.open(0, cv::CAP_V4L2);
  if (!choosen_cap.isOpened()) {
    std::cerr << "Ошибка: Не удалось открыть камеру." << std::endl;
    return -1;
  }

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