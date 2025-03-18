#include <opencv2/opencv.hpp>
#include <iostream>

int main() {

    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "Ошибка: Не удалось открыть камеру." << std::endl;
        return -1;
    }

    cv::Mat frame;

    while (true) {
      cap >> frame;
      cv::imshow("Webcam", frame);
      if (cv::waitKey(1) == 'q') {
          break;
      }
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}