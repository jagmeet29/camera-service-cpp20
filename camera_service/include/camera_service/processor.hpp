#pragma once

#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"

namespace camera_service {

struct ProcessorConfig {
    std::chrono::milliseconds processing_delay{100};
};

void processing_loop(BoundedFrameQueue& queue, PipelineMetrics& metrics, ProcessorConfig config);

}  // namespace camera_service
