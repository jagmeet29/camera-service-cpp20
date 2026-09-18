#include "camera_service/capture.hpp"

#include <chrono>
#include <utility>

#include <opencv2/videoio.hpp>

namespace camera_service {

void capture_loop(
    BoundedFrameQueue& queue,
    PipelineMetrics& metrics,
    volatile std::sig_atomic_t& stop_requested,
    CaptureConfig config)
{
    cv::VideoCapture capture;
    capture.open(config.device_index, cv::CAP_ANY);

    if (!capture.isOpened()) {
        ++metrics.read_failures;
        stop_requested = 1;
        return;
    }

    cv::Mat image;

    while (!stop_requested) {
        if (!capture.read(image) || image.empty()) {
            ++metrics.read_failures;
            stop_requested = 1;
            break;
        }

        const auto captured_at = std::chrono::steady_clock::now();
        const std::uint64_t sequence_number =
            metrics.captured_frames.fetch_add(1) + 1;

        Frame frame{
            image.clone(),
            sequence_number,
            captured_at
        };

        if (!queue.push(std::move(frame))) {
            ++metrics.dropped_frames;
        }
    }
}

}  // namespace camera_service