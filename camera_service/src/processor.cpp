#include "camera_service/processor.hpp"

#include <algorithm>
#include <chrono>
#include <thread>

namespace camera_service {

void processing_loop(BoundedFrameQueue& queue, PipelineMetrics& metrics) {
    Frame frame;
    while (queue.wait_and_pop(frame)) {
        const auto processing_started = std::chrono::steady_clock::now();
        const double frame_age_ms =
            std::chrono::duration<double, std::milli>(
                processing_started - frame.captured_at)
                .count();

        metrics.last_frame_age_ms.store(frame_age_ms);
        {
            std::lock_guard<std::mutex> lock(metrics.age_mutex);
            metrics.frame_age_sum_ms += frame_age_ms;
            ++metrics.frame_age_samples;
            metrics.frame_age_max_ms =
                std::max(metrics.frame_age_max_ms, frame_age_ms);
        }

        // Intentional temporary overload used to exercise the queue policy.
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        ++metrics.processed_frames;
    }
}

}  // namespace camera_service
