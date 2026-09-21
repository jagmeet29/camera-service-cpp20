#pragma once

#include <csignal>
#include <atomic>
#include <string>

#include "camera_service/bounded_frame_queue.hpp"
#include "camera_service/pipeline_metrics.hpp"

namespace camera_service {

    enum class CameraState {
        Starting,
        Streaming,
        Disconnected,
        Stopping,
        Stopped
    };

    struct CaptureConfig {
        std::string source_url;
    };

    void capture_loop(
        BoundedFrameQueue& queue,
        PipelineMetrics& metrics,
        std::atomic<bool>& stop_requested,
        CaptureConfig config);

}  // namespace camera_service