#include "camera_service/capture.hpp"

#include <chrono>
#include <utility>
#include <atomic>
#include <iostream>
#include <thread>

#include <opencv2/videoio.hpp>

namespace camera_service {

void capture_loop(
    BoundedFrameQueue& queue,
    PipelineMetrics& metrics,
    std::atomic<bool>& stop_requested,
    CaptureConfig config)
{

    CameraState state = CameraState::Starting;
    std::cerr << "camera_state=Starting\n";

    cv::VideoCapture capture;
    cv::Mat image;

    while (!stop_requested.load()) {
        // A failed read releases the device, so the next iteration reopens it.
        if (!capture.isOpened()) {
            capture.open(
                config.source_url,
                cv::CAP_FFMPEG,
                {
                    cv::CAP_PROP_OPEN_TIMEOUT_MSEC, 3000,
                    cv::CAP_PROP_READ_TIMEOUT_MSEC, 3000
                }
            );

            if (!capture.isOpened()) {
                ++metrics.open_failures;
                if (state != CameraState::Disconnected) {
                    state = CameraState::Disconnected;
                    std::cerr << "camera_state=Disconnected\n";
                }

                for (int i = 0; i < 20 && !stop_requested.load(); ++i) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                }
                continue;
            }
        }

        // Ctrl-C may have arrived while the camera was opening.
        if (stop_requested.load()) {
            break;
        }

        if (!capture.read(image) || image.empty()) {
            ++metrics.read_failures;
            if (state != CameraState::Disconnected) {
                state = CameraState::Disconnected;
                std::cerr << "camera_state=Disconnected\n";
            }
            capture.release();

            for (int i = 0; i < 20 && !stop_requested.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
            continue;
        }

        if (state != CameraState::Streaming) {
            state = CameraState::Streaming;
            std::cerr << "camera_state=Streaming\n";
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

    state = CameraState::Stopping;
    std::cerr << "camera_state=Stopping\n";

    capture.release();

    state = CameraState::Stopped;
    std::cerr << "camera_state=Stopped\n";
}

}  // namespace camera_service
