#include <chrono>
#include <iostream>
#include <stdexcept>

#include "chessboard_filter_encoder.hpp"

#include "opencv2/calib3d.hpp"
#include "opencv2/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

ChessboardFilterEncoder::ChessboardFilterEncoder(VideoOptions const *options) : Encoder(options), abort_(false)
{
	LOG(2, "Opened ChessboardFilterEncoder");
	output_thread_ = std::thread(&ChessboardFilterEncoder::outputThread, this);
}

ChessboardFilterEncoder::~ChessboardFilterEncoder()
{
	abort_ = true;
	output_thread_.join();
	LOG(2, "ChessboardFilterEncoder closed");
}

// Push the buffer onto the output queue to be "encoded" and returned.
void ChessboardFilterEncoder::EncodeBuffer(int fd, size_t size, void *mem, StreamInfo const &info, int64_t timestamp_us)
{
	uint16_t *ptr = (uint16_t *)mem;

	cv::Mat raw_image = cv::Mat(info.height, info.width, CV_16U, ptr, info.stride);
	cv::Mat raw_image_8bit;
	raw_image.convertTo(raw_image_8bit, CV_8U, 1 / 256.0);

	cv::Mat image;
	cv::cvtColor(raw_image_8bit, image, cv::COLOR_BayerRG2RGB);

	if (image.empty())
	{
		std::cout << "Could not open or find the image" << std::endl;
	}
	cv::Mat green;
	cv::extractChannel(image, green, 1);

	cv::Size2i inner_corner_dim(7, 10);

	cv::Mat detected_corners;
	bool success = cv::findChessboardCorners(green, inner_corner_dim, detected_corners, cv::CALIB_CB_FAST_CHECK);
	if (success)
	{
		cv::cornerSubPix(green, detected_corners, cv::Size2i(5, 5), cv::Size2i(-1, -1),
						 cv::TermCriteria(cv::TermCriteria::EPS | cv::TermCriteria::MAX_ITER, 30, 0.001));
		cv::Mat sharpness_array;
		cv::Scalar sharpness_measure =
			cv::estimateChessboardSharpness(green, inner_corner_dim, detected_corners, 0.8, false, sharpness_array);
		std::cout << sharpness_measure << std::endl;

		// cv::drawChessboardCorners(image, inner_corner_dim, detected_corners, success);
		std::lock_guard<std::mutex> lock(output_mutex_);
		OutputItem item = { mem, size, timestamp_us };
		output_queue_.push(item);
		output_cond_var_.notify_one();
	}
}

// Realistically we would probably want more of a queue as the caller's number
// of buffers limits the amount of queueing possible here...
void ChessboardFilterEncoder::outputThread()
{
	OutputItem item;
	while (true)
	{
		{
			std::unique_lock<std::mutex> lock(output_mutex_);
			while (true)
			{
				using namespace std::chrono_literals;
				if (!output_queue_.empty())
				{
					item = output_queue_.front();
					output_queue_.pop();
					break;
				}
				else
					output_cond_var_.wait_for(lock, 200ms);
				if (abort_)
					return;
			}
		}
		// Ensure the input done callback happens before the output ready callback.
		// This is needed as the metadata queue gets pushed in the former, and popped
		// in the latter.
		input_done_callback_(nullptr);
		output_ready_callback_(item.mem, item.length, item.timestamp_us, true);
	}
}
