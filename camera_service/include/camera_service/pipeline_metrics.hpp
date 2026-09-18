#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>

namespace camera_service {

struct PipelineMetrics {
    std::atomic<std::uint64_t> processed_frames{0};
    std::atomic<double> last_frame_age_ms{0.0};

    // These values form one interval statistic and must change together.
    std::mutex age_mutex;
    double frame_age_sum_ms = 0.0;
    double frame_age_max_ms = 0.0;
    std::uint64_t frame_age_samples = 0;
};

}  // namespace camera_service
