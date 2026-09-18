#pragma once

#include <condition_variable> // for effecient sleep and wakeup
#include <cstddef> // for types like uint64_t or size_t
#include <deque> // queue
#include <mutex> // multiprocess safy locking

#include "camera_service/frame.hpp"

namespace camera_service {

class BoundedFrameQueue {
public:
    explicit BoundedFrameQueue(std::size_t capacity);

    // Returns true when no frame was dropped. A false result means the queue
    // was full and its oldest frame was discarded before enqueuing this one.
    bool push(Frame frame);
    bool wait_and_pop(Frame& output);
    void close();
    std::size_t size();
    std::size_t take_high_water_mark();

private:
    std::deque<Frame> frames_;
    const std::size_t capacity_;
    std::mutex mutex_;
    std::condition_variable not_empty_;
    bool closed_ = false;
    std::size_t high_water_mark_ = 0;
    
};

}  // namespace camera_service
