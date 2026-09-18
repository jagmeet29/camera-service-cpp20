#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"
#include "camera_service/processor.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {

volatile std::sig_atomic_t stop_requested = 0;

void handle_signal(int) {
    stop_requested = 1;
}

}  // namespace

int main() {
    using camera_service::BoundedFrameQueue;
    using camera_service::Frame;
    using camera_service::PipelineMetrics;

    BoundedFrameQueue queue(4);
    PipelineMetrics metrics;
    std::uint64_t frame_number = 0;
    std::uint64_t dropped_frames = 0;
    std::uint64_t previous_processed_frames = 0;
    std::uint64_t frames_in_interval = 0;

    std::signal(SIGINT, handle_signal);

    cv::VideoCapture capture;
    capture.open(0, cv::CAP_ANY);
    if (!capture.isOpened()) {
        std::cerr << "ERROR! Unable to open camera\n";
        return 1;
    }

    std::thread processor([&queue, &metrics] {
        camera_service::processing_loop(queue, metrics);
    });

    std::cout << "Start grabbing\n"
              << "Press q in the video window or Ctrl-C in the terminal to terminate\n";

    const auto service_started = std::chrono::steady_clock::now();
    auto interval_started = service_started;
    cv::Mat image;

    while (!stop_requested) {
        if (!capture.read(image) || image.empty()) {
            std::cerr << "ERROR! blank frame grabbed\n";
            break;
        }

        const auto captured_at = std::chrono::steady_clock::now();
        ++frame_number;
        ++frames_in_interval;

        Frame captured_frame{image.clone(), frame_number, captured_at};
        if (!queue.push(std::move(captured_frame))) {
            ++dropped_frames;
        }

        const auto now = std::chrono::steady_clock::now();
        const double elapsed_seconds =
            std::chrono::duration<double>(now - service_started).count();
        const double interval_seconds =
            std::chrono::duration<double>(now - interval_started).count();

        if (interval_seconds >= 1.0) {
            const std::uint64_t processed_total = metrics.processed_frames.load();
            const std::uint64_t processed_this_interval =
                processed_total - previous_processed_frames;
            previous_processed_frames = processed_total;

            double frame_age_sum_ms = 0.0;
            double frame_age_max_ms = 0.0;
            std::uint64_t frame_age_samples = 0;
            {
                std::lock_guard<std::mutex> lock(metrics.age_mutex);
                frame_age_sum_ms = metrics.frame_age_sum_ms;
                frame_age_max_ms = metrics.frame_age_max_ms;
                frame_age_samples = metrics.frame_age_samples;
                metrics.frame_age_sum_ms = 0.0;
                metrics.frame_age_max_ms = 0.0;
                metrics.frame_age_samples = 0;
            }

            const double frame_age_avg_ms = frame_age_samples == 0
                ? 0.0
                : frame_age_sum_ms / static_cast<double>(frame_age_samples);

            std::cout << "metrics total_frames=" << frame_number
                      << " capture_fps="
                      << static_cast<double>(frames_in_interval) / interval_seconds
                      << " dropped_frames=" << dropped_frames
                      << " processing_fps="
                      << static_cast<double>(processed_this_interval) / interval_seconds
                      << " queue_size=" << queue.size()
                      << " frame_age_ms=" << metrics.last_frame_age_ms.load()
                      << " frame_age_avg_ms=" << frame_age_avg_ms
                      << " frame_age_max_ms=" << frame_age_max_ms << '\n';

            frames_in_interval = 0;
            interval_started = now;
        }

        const double display_fps = elapsed_seconds == 0.0
            ? 0.0
            : static_cast<double>(frame_number) / elapsed_seconds;
        cv::putText(image, "FPS: " + std::to_string(display_fps), cv::Point(50, 50),
                    cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Live", image);

        if (cv::waitKey(5) == 'q') {
            stop_requested = 1;
        }
    }

    queue.close();
    processor.join();
    return 0;
}
