#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
#include <boost/asio.hpp>
#include <thread>

namespace asio = boost::asio;
using asio::ip::tcp;

#define PATH_TO_VIDEO "C:\\Users\\User\\Downloads\\DSC_7681.MP4"

int main() {
	setlocale(LC_ALL, "russian");

	boost::system::error_code ec;
	asio::io_context io_context;

	tcp::socket socket(io_context);

	tcp::endpoint endpoint(asio::ip::make_address("192.168.0.100"), 8080);

	socket.connect(endpoint, ec);

	if (ec.failed()) {
		std::cerr << ec.message() << std::endl;
		return ec.value();
	}

	cv::VideoCapture cap(PATH_TO_VIDEO);

	std::vector<int> params = { cv::IMWRITE_JPEG_QUALITY, 70 };

	const int fps = cap.get(cv::CAP_PROP_FPS);

	while (cap.isOpened() && socket.is_open()) {
		auto start = std::chrono::system_clock::now();
		cv::Mat frame;
		cv::Mat small_frame;
		std::vector<uchar> data;

		cap >> frame;
		if (frame.empty()) break;
		cv::resize(frame, small_frame, cv::Size(640, 360));
		cv::imencode(".jpg",	small_frame, data, params);

		for (size_t i = 0; i < data.size(); i += 1024) {
			socket.write_some(asio::buffer(data.data() + i, data.size() - i < 1024 ? data.size() % 1024 : 1024), ec);
			if (ec) {
				std::cerr << "Failed to send frame: " << ec.message() << std::endl;
				return ec.value();
			}
		}

		if (ec) {
			std::cerr << ec.message() << std::endl;
			return ec.value();
		}

		auto end = std::chrono::system_clock::now();
		std::chrono::system_clock::duration dur = end - start;
		int dur_ms = std::chrono::duration_cast<std::chrono::milliseconds>(dur).count();
		int time_to_sleep = 1000 / fps - dur_ms;
		std::cout << data.size() << std::endl;
		if (time_to_sleep > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(time_to_sleep));
	}
}