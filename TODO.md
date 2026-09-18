# Camera service roadmap

Goal: build a Linux C++ camera component that delivers fresh frames, remains bounded under overload, recovers from camera failures, exits safely, and produces useful metrics.

## Progress estimate

- **Overall completion:** approximately **35%**.
- **Remaining focused implementation time:** approximately **21–37 hours**.
- **At 4–6 hours per week:** approximately **5–8 weeks**.

The largest remaining pieces are restoring threaded metrics reporting, camera
disconnect/reconnect recovery, sanitizer/debug verification, and documented
experiments. The estimate includes learning and debugging time, not only typing
code.

## GitHub checkpoints

- [ ] **Checkpoint 1 — first push:** Restore the main-thread reporter loop; build successfully; run the producer/consumer overload test; commit the working two-thread pipeline and this TODO.
- [ ] **Checkpoint 2 — shareable progress:** Make processing delay configurable; add queue high-water/read-failure metrics; publish measured overload results in `README.md`.
- [ ] **Checkpoint 3 — portfolio-ready public repository:** Demonstrate disconnect/reconnect recovery, clean shutdown in all states, sanitizer/GDB verification, architecture diagram, and a short demo video.

It is safe to push to a private GitHub repository at Checkpoint 1. Link the
repository publicly on a resume only after Checkpoint 3.

## Milestone 1 — sequential capture baseline

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
- [ ] Publish the actual commands, results, and interpretation in `README.md`.

## Milestone 4 — camera failure and recovery

- [ ] Introduce explicit service states: `Starting`, `Streaming`, `Disconnected`, `Stopping`, `Stopped`.
- [ ] On failed read, release the capture device and transition to `Disconnected`.
- [ ] Retry opening the camera using a bounded backoff delay.
- [ ] On successful reopening, return to `Streaming`.
- [ ] Count reconnect attempts and successful reconnects.
- [ ] Measure and report recovery time after a physical disconnect/reconnect test.

## Milestone 5 — shutdown and verification

- [x] Make Ctrl-C request shutdown without doing cleanup inside the signal handler.
- [x] Wake blocked consumers during shutdown.
- [x] Join the processing thread before `main` returns.
- [ ] Verify shutdown during capture, during slow processing, and while disconnected.
- [ ] Build and run with AddressSanitizer and UndefinedBehaviorSanitizer.
- [ ] Debug at least one real issue with GDB or sanitizer output; document the cause and fix.

## Portfolio delivery

- [ ] Add build and run instructions, dependencies, and camera permissions to `README.md`.
- [ ] Add an architecture diagram showing capture, bounded queue, processor, metrics, and recovery.
- [ ] Add a short demo video: normal operation, overload, disconnect/reconnect, clean shutdown.
- [ ] Add a results table with measured throughput, drops, frame age, memory behavior, and recovery time.
- [ ] Write a resume bullet using only the measurements you actually obtained.
