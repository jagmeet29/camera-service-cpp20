#pragma once

#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"

namespace camera_service {

void processing_loop(BoundedFrameQueue& queue, PipelineMetrics& metrics);

}  // namespace camera_service
