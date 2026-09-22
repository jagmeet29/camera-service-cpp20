#include "camera_service/bounded_frame_queue.hpp"

#include <cassert>
#include <iostream>

int main() {
    camera_service::BoundedFrameQueue queue(2);

    // Empty images are enough: we are testing frame order.
    camera_service::Frame first{};
    first.sequence_number = 1;

    camera_service::Frame second{};
    second.sequence_number = 2;

    camera_service::Frame third{};
    third.sequence_number = 3;

    queue.push(first);
    queue.push(second);
    queue.push(third);

    // Three frames entered, but only two should remain.
    assert(queue.size() == 2);

    // No more frames will arrive. Existing frames can still be popped.
    // Closing also prevents this test hanging if the queue is empty.
    queue.close();

    camera_service::Frame output{};

    bool received = queue.wait_and_pop(output);
    assert(received);
    assert(output.sequence_number == 2);

    received = queue.wait_and_pop(output);
    assert(received);
    assert(output.sequence_number == 3);

    assert(queue.size() == 0);

    std::cout << "PASS: queue drops the oldest frame\n";

    received = queue.wait_and_pop(output);
    assert(!received);

    std::cout << "PASS: drop-oldest and closed-empty behavior\n";
}