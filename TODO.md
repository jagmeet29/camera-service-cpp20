# Camera service roadmap

Goal: build a Linux C++ camera component that delivers fresh frames, remains bounded under overload, recovers from camera failures, exits safely, and produces useful metrics.

## Progress estimate

- **Last reviewed:** 2026-09-22, against the current source and user-reported tests.
- **Overall completion:** approximately **70%**; **30% remaining**.
- **Remaining focused implementation and learning time:** approximately **12–22 hours**.
- **At 4–6 hours per week:** approximately **3–6 weeks**.

This is an effort estimate, not a percentage of checked boxes. The bounded
pipeline, interval metrics, configurable delay, and initial README are working.
Basic phone-stream recovery and lock-free atomic shutdown signaling are now
implemented. Open/read timeouts are configured, failure counters are separated,
and basic queue tests pass according to the reported run. Remaining work includes timeout verification, input validation,
recovery measurements, failure tests, and reproducible measurements. Driver behavior and
debugging may change the estimate.

### Remaining timeline (learning time included)

1. Automated test setup, queue edge cases, and input validation: **3–5 hours**.
2. Failure/shutdown verification, exception handling, and sanitizer runs: **5–9 hours**.
3. Reproducible performance/memory/recovery results, README, and demo: **4–8 hours**.

These are planning ranges, not deadlines. Optional reconnect counters and capped
backoff are deferred and are not prerequisites for the first portfolio version.

## Immediate next task

- [x] Replace the shared stop flag with `std::atomic<bool>` and verify at compile
  time that it is always lock-free; keep the signal handler limited to setting it.
- [x] Select FFmpeg explicitly for the phone URL and pass 3000 ms open/read timeouts.
- [ ] Register `queue_test` with CTest and set a test timeout so a regression cannot hang the test runner indefinitely.
- [ ] Verify Ctrl-C during genuinely stalled network I/O; connection-refused tests do not prove timeout behavior.
- [ ] Validate delay arguments fully: reject text, trailing characters, negative
  numbers, and out-of-range values with a useful error instead of an uncaught exception.

## GitHub checkpoints

- [x] **Checkpoint 1 — first-push code milestone:** Reporter restored and producer/consumer overload test demonstrated. Remote corrected to `jagmeet29/camera-service-cpp20`; latest remote publication was not checked during this review.
- [x] Exclude local `build/` and `build-camera/` directories from Git tracking without deleting local build files.
- [x] **Checkpoint 2 — shareable progress:** Processing delay is configurable; queue high-water/read-failure metrics and initial overload results are documented in `README.md`.
- [ ] **Checkpoint 3 — portfolio-ready public repository:** Demonstrate disconnect/reconnect recovery, clean shutdown in all states, sanitizer/GDB verification, architecture diagram, and a short demo video.

The project can be shared now as work in progress. Checkpoint 3 is the target
for demonstrating recovery and reliability in a portfolio.

## Milestone 1 — sequential capture baseline

These are historical baseline achievements. The current service is headless;
GUI display and the `q` exit were removed when capture moved to its own thread.

- [x] Open webcam index `0` with OpenCV.
- [x] Reject an unavailable camera with a useful error.
- [x] Read and display frames.
- [x] Overlay an FPS value on the displayed frame.
- [x] Exit with `q` in the video window.
- [x] Exit with Ctrl-C in the terminal using `SIGINT`.
- [x] Detect a failed read or empty frame and exit with an error.
- [x] Print terminal metrics once per second: total frames and interval capture FPS.
- [ ] Run the baseline for 30 seconds and save its terminal output.
- [ ] Record camera model, resolution, chosen backend, and measured FPS in `README.md`.

## Milestone 2 — bounded producer/consumer pipeline

- [x] Define a `Frame` type containing `cv::Mat`, sequence number, and a monotonic capture timestamp.
- [x] Define a `BoundedFrameQueue` interface and its private shared state.
- [x] Validate that queue capacity is greater than zero at construction.
- [x] Implement producer-side `push`: mutex-protected, drop-oldest when full, then notify a waiting consumer.
- [x] Implement consumer-side `wait_and_pop` using `std::condition_variable`.
- [x] Implement `close` and `size` using the same mutex.
- [x] Move camera capture into one producer thread.
- [x] Split capture and processing code into dedicated headers and implementation files.
- [x] Move processing into one consumer thread.
- [x] Create a bounded queue protected by a mutex and condition variable.
- [x] Pick a drop policy: drop oldest frame when full (preferred for fresh telepresence/robotics data).
- [x] Clone each captured `cv::Mat` before queueing, then move the owned `Frame` into the queue.
- [x] Add a temporary 100 ms artificial processing delay to reproduce overload.
- [x] Verify under overload that the capacity-4 queue stays at or below 4 frames.
- [x] Make the processing delay configurable with `--processing-delay-ms`.

## Milestone 3 — useful service metrics

- [x] Safely count processed frames from the consumer thread using an atomic counter.
- [x] Restore one-second main-thread reporting after the producer-thread refactor.
- [x] Report capture FPS, processing FPS, current queue size, and dropped-frame count.
- [x] Report queue high-water mark and read-failure count.
- [x] Report latest frame age: `processing_start_time - capture_timestamp`.
- [x] Report average and maximum frame age for each interval.
- [x] Verify zero-delay behavior: ~30 FPS capture/processing, zero drops, empty queue, and sub-millisecond frame age.
- [ ] Run overload experiments with several processing delays and queue capacities.
- [x] Publish initial 0 ms and 100 ms delay results and run commands in `README.md`.
- [ ] Save raw logs, camera/backend/resolution details, and results for the expanded experiment set.

## Milestone 4 — camera failure and recovery

- [x] Introduce explicit service states: `Starting`, `Streaming`, `Disconnected`, `Stopping`, `Stopped`.
- [x] On failed read, release the capture device and transition to `Disconnected`.
- [x] Retry opening after a fixed one-second delay, checking the stop flag every 50 ms during that delay.
- [ ] Optional/deferred: add capped backoff for repeated failures (currently the retry delay is fixed).
- [x] After reopening and receiving a valid frame, return to `Streaming`.
- [x] Demonstrate phone IP-camera stream loss and recovery without restarting the service.
- [ ] Optional/deferred: count reconnect attempts and successful reconnects.
- [ ] Measure and report recovery time for the phone-stream restart test; distinguish failure detection time from recovery after the source becomes available.
- [x] Separate open failures from read failures and report both counters.

Reported phone test: `Streaming -> Disconnected -> Streaming`, followed by a
second disconnection and Ctrl-C reaching `Stopping -> Stopped`. This verifies
basic network-stream recovery, not USB hotplug behavior or a shutdown deadline.
No timed recovery measurement has been recorded yet.

## Milestone 5 — shutdown and verification

- [x] Make Ctrl-C request shutdown without doing cleanup inside the signal handler.
- [x] Wake blocked consumers during shutdown.
- [x] Join the processing thread before `main` returns.
- [x] Join the capture thread before closing the queue.
- [x] Use atomic loads/stores for the shared stop flag; require lock-free operations for signal-handler use.
- [ ] Handle worker exceptions without terminating the process with joinable threads.
- [x] Measure cleanup time from main observing the stop request until both threads are joined.
- [x] Observe shutdown while streaming with slow processing and while disconnected.
- [ ] Document and test camera read/open blocking limitations; do not claim a hard shutdown deadline.
- [x] Observe Ctrl-C shutdown while disconnected in the reported phone test.
- [x] Add a standalone `queue_test` executable with assertion checks (run with assertions enabled).
- [x] Test capacity 2 with frames 1, 2, 3: size stays 2 and popped frames are 2 then 3.
- [x] Test draining a closed queue and returning false when it is closed and empty.
- [ ] Test invalid capacity and push-after-close behavior; clarify the push return contract.
- [ ] Test that closing the queue wakes a waiting consumer (not covered by the single-threaded test).
- [ ] Build and run with AddressSanitizer and UndefinedBehaviorSanitizer.
- [ ] Check thread synchronization with ThreadSanitizer where supported.
- [ ] Debug at least one real issue with GDB or sanitizer output; document the cause and fix.

Reported cleanup measurements: **8.88389 ms** while disconnected and **162.616 ms**
while streaming under artificial processing load. These are individual samples,
not worst-case bounds or exact Ctrl-C-to-exit times. The last printed queue size
does not establish queue size at shutdown. Silent-network timeout testing remains pending.

## Portfolio delivery

- [x] Add build/run instructions and dependencies to `README.md`.
- [ ] Document camera permission troubleshooting.
- [x] Add a Mermaid diagram showing capture, queue, processor, and metrics.
- [ ] Extend the architecture documentation when recovery is implemented.
- [ ] Update README for the phone URL, recovery behavior, and current limitations.
  Explain that frame age starts after OpenCV returns a frame, not at the phone's
  exposure time; the last age value remains stale when no new frames arrive.
- [ ] Add a short demo video: normal operation, overload, disconnect/reconnect, clean shutdown.
- [x] Add initial throughput, drop, queue, and frame-age results.
- [ ] Add measured memory behavior and recovery time to the results table.
- [ ] Write a resume bullet using only the measurements you actually obtained.
