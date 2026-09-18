#include "camera_service/bounded_frame_queue.hpp"

#include <stdexcept>
#include <utility>

namespace camera_service {

BoundedFrameQueue::BoundedFrameQueue(std::size_t capacity)
    : capacity_(capacity) {
    if (capacity_ == 0) {
        throw std::invalid_argument("Queue capacity must be greater than zero");
    }
}

bool BoundedFrameQueue::push(Frame frame) {
    bool not_dropped = true;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_) {
            return true;
        }
        if (frames_.size() == capacity_) {
            frames_.pop_front();
            not_dropped = false;
        }
        frames_.push_back(std::move(frame));
    }
    not_empty_.notify_one();
    return not_dropped;
}

bool BoundedFrameQueue::wait_and_pop(Frame& output) {
    std::unique_lock<std::mutex> lock(mutex_);
    not_empty_.wait(lock, [this] {
        return closed_ || !frames_.empty();
    });
    if (frames_.empty()) {
        return false;
    }
    output = std::move(frames_.front());
    frames_.pop_front();
    return true;
}

void BoundedFrameQueue::close() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        closed_ = true;
    }
    not_empty_.notify_all();
}

std::size_t BoundedFrameQueue::size() {
    std::lock_guard<std::mutex> lock(mutex_);
    return frames_.size();
}

}  // namespace camera_service
