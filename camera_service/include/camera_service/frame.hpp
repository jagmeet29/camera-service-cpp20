#pragma once

#include <chrono> // for time
#include <cstdint> // for type (uint64_t)

#include <opencv2/core.hpp>

namespace camera_service {

struct Frame {
    cv::Mat image;
    std::uint64_t sequence_number;
    std::chrono::steady_clock::time_point captured_at;
};

}  // namespace camera_service
