#pragma once

#include <csignal>

#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"

namespace camera_service {

struct CaptureConfig {
    int device_index = 0;
};

void capture_loop(
    BoundedFrameQueue& queue,
    PipelineMetrics& metrics,
    volatile std::sig_atomic_t& stop_requested,
    CaptureConfig config);

}  // namespace camera_service