#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"
#include "camera_service/processor.hpp"
#include "camera_service/capture.hpp"

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <thread>
#include <utility>
#include <mutex>
#include <string>
#include <string_view>
#include <atomic>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

namespace {

static_assert(std::atomic<bool>::is_always_lock_free, "Signal handler requires a lock-free atomic bool");

std::atomic<bool> stop_requested{false};

void handle_signal(int) {
    stop_requested.store(true);
}

}  // namespace

int main(int argc, char* argv[]) {
    camera_service::ProcessorConfig processor_config{};

    if (argc == 3 && std::string_view(argv[1]) == "--processing-delay-ms") {
        const int delay_ms = std::stoi(argv[2]);

        if (delay_ms < 0) {
            std::cerr<<"Delay must not be negative\n";
            return 2;
        }

        processor_config.processing_delay = std::chrono::milliseconds(delay_ms);
    } else if (argc != 1) {
        std::cerr<<"useage:"
            << argv[0]
            << " [--processing-delay-ms NUMBER\n]";
        return 2;
    }

    using camera_service::BoundedFrameQueue;
    using camera_service::Frame;
    using camera_service::PipelineMetrics;

    BoundedFrameQueue queue(4);
    PipelineMetrics metrics;

    std::signal(SIGINT, handle_signal);


    std::thread processor([&queue, &metrics, processor_config] {
        camera_service::processing_loop(queue, metrics, processor_config);
    });

    camera_service::CaptureConfig capture_config{"http://192.168.1.35:8080/videofeed"};

    std::thread capture_thread([&queue, &metrics, capture_config] {
        camera_service::capture_loop(queue, metrics, stop_requested, capture_config);
    });

    std::cout << "Start grabbing\n"
              << "Press Ctrl-C in the terminal to terminate\n";
    

    auto interval_started = std::chrono::steady_clock::now();
    std::uint64_t previous_processed_frame = 0;
    std::uint64_t previous_captured_frame = 0;

    while (!stop_requested.load())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        auto now = std::chrono::steady_clock::now();
        const double interval_seconds = std::chrono::duration<double>(now - interval_started).count();
        if (interval_seconds < 1.0) {
            continue;
        }
       
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

        const double frame_age_avg_ms = frame_age_samples == 0 ? 0.0 : frame_age_sum_ms / static_cast<double>(frame_age_samples);
        const std::uint64_t processed_total = metrics.processed_frames.load();
        const std::uint64_t processed_this_interval = processed_total - previous_processed_frame;
        const std::uint64_t captured_total = metrics.captured_frames.load();
        const std::uint64_t captured_this_interval = captured_total - previous_captured_frame;
        const std::size_t queue_high_water = queue.take_high_water_mark();

        previous_processed_frame = processed_total;
        previous_captured_frame = captured_total;
        std::cout << "metrics capture_fps="
                  << static_cast<double>(captured_this_interval) / interval_seconds
                  << " processing_fps="
                  << static_cast<double>(processed_this_interval) / interval_seconds
                  << " dropped_frames=" << metrics.dropped_frames.load()
                  << " queue_size=" << queue.size()
                  << " frame_age_ms=" << metrics.last_frame_age_ms.load()
                  << " frame_age_avg_ms=" << frame_age_avg_ms
                  << " frame_age_max_ms=" << frame_age_max_ms
                  << " queue_high_water=" << queue_high_water
                  << " read_failures=" << metrics.read_failures.load()
                  << " open_failures=" << metrics.open_failures.load()
                  << '\n';

        interval_started = now;

    }
    const auto shutdown_started = std::chrono::steady_clock::now();
    
    capture_thread.join();
    queue.close(); 
    processor.join();

    
    const auto shutdown_finished = std::chrono::steady_clock::now();

    const double shutdown_ms =
        std::chrono::duration<double, std::milli>(
            shutdown_finished - shutdown_started
        ).count();

    std::cout << "shutdown_ms=" << shutdown_ms << '\n';
    return 0;
}
