# Repeatable Timer

- A tiny, header‑only, self‑rescheduling timer that works with **ASIO Stand‑alone**
- Author: *Dean Sellers*
- SPDX-License-Identifier: MIT

---

## 1. Overview

`RepeatingTimer` is a small, template‑driven wrapper around `asio::steady_timer` that keeps a timer alive until the last `shared_ptr` to the object is destroyed. The timer automatically reschedules itself after each tick and asses a user‑supplied **context** object to the callback on every invocation.

> **Why use it?**
> - No boilerplate for manual `async_wait` loops.
> - Thread‑safe access to the context (via a `std::mutex`).
> - Header‑only, no separate compilation required.

---

## 2. Features

| Feature | Benefit |
|---------|-------------------|
| **Header‑only** | Just drop the `repeatable_timer.hpp` header into your project. |
| **Template context** | Pass any type as context: counter, struct, smart pointer, … |
| **Thread‑safe** | `std::mutex` protects the context while the user callback runs. |
| **Self‑rescheduling** | The timer reschedules automatically until you call `cancel()` or the object dies. |
| **Reschedule/Preempt** | The timer can be rescheduled permanently, just once or trigger immediately. |
| **ASIO‑standalone** | Uses `asio::steady_timer` (no Boost dependency). |
| **No external libs** | Only standard library + ASIO. |

---

## 3. Installation

1. **Get the header**

    ```
    # Option A – copy the single header
    cp repeatable_timer.hpp /path/to/your/include/
    ```

    or

    ```
    # Option B – git submodule
    git submodule add https://github.com/d-o/repeatable-timer.git
    ```

2. **Make sure ASIO is available**
   *Standalone ASIO* can be pulled via CMake’s `FetchContent` (shown later) or you can simply drop the `asio` directory into your project.

3. **Include**

    ```cpp
    #include "repeatable_timer.hpp"
    ```

---

## 4. Usage

### 4.1 Basic Example

```cpp
#include "repeatable_timer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    asio::io_context io;

    // Create a timer that prints a counter every second.
    auto timer = RepeatingTimer<int>::create(
        io,
        [](int& counter) {
            std::cout << "Tick #" << ++counter << '\n';
        },
        std::chrono::seconds(1),
        std::make_shared<int>(0)  // initial counter value
    );

    // Run the io_context in its own thread
    std::thread io_thread([&io]{ io.run(); });

    // Let it tick 5 times.
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // When the timer goes out of scope it stops automatically.
    timer.reset();  // destroy the timer

    io_thread.join();
    std::cout << "Timer stopped.\n";
}
```

> **Output**

    Tick #1
    Tick #2
    Tick #3
    Tick #4
    Tick #5
    Timer stopped.

### 4.2 Custom Context

```cpp
struct Stats { int total = 0; double sum = 0.0; };

auto timer = RepeatingTimer<Stats>::create(
    io,
    [](Stats& s) {
        double value = 1.0;                 // some value you generate
        s.total++;
        s.sum += value;
        std::cout << "Total: " << s.total
                    << "  Avg: " << s.sum / s.total << '\n';
    },
    std::chrono::seconds(2),
    std::make_shared<Stats>()   // default‑constructed context
);
```

### 4.3 Cancellation

If you need to stop the timer before the object dies:

    timer->cancel();   // will prevent further rescheduling

### 4.4 Rescheduling

You can reschedule the timer

    timer->reschedule(); // Will just make it wait for the set period again
    timer->reschedule(period); // Will set it to the new period for one run
    timer->reschedule(0); // Run once ASAP
    timer->reschedule(period, true) // Reschedule and save the new period

### 4.5 Thread‑Safety

`RepeatingTimer` holds a `std::mutex`. The mutex is locked if the context is valid while invoking user callbacks, so the callback runs atomically with respect to other invocations. If you require more control over resource locking use a `nullptr` context and manage your context using a lambda function.

---

## 5. API Reference

```cpp
namespace asio { /* ... */ }   // ASIO Stand‑alone

template<class Context>
class RepeatingTimer
{
public:
    // Type of the callback that receives a reference to the context
    using Callback = std::function<void(Context&)>;

    // Factory that creates and schedules the timer, milliseconds or seconds
    static std::shared_ptr<RepeatingTimer>
    static std::shared_ptr<RepeatingTimer> create(
        asio::io_context& io,
        Callback cb,
        std::chrono::milliseconds period,
        std::shared_ptr<Context> ctx,
        Callback cb_once = nullptr,
        Callback cb_last = nullptr);

    // Cancel the timer immediately
    void cancel();

    // Reschedule
    void reschedule(std::chrono::milliseconds newPeriod, bool saveNew = false);
    void reschedule(std::chrono::seconds newPeriod, bool saveNew = false);
    void reschedule();

    // Destructor automatically cancels the timer
    ~RepeatingTimer();
};
```

---

**Tests**

    cd test
    mkdir build && cd build
    cmake ..
    cmake --build .
    ./repeating_timer_test

**Test output**

    Testing auto destruction.
        Tick #1
        Tick #2
        Tick #3
        Tick #4
        Tick #5
        Timer destroyed.
    Testing cancelling.
        Tick #1
        Tick #2
        Tick #3
        Tick #4
        Tick #5
        Timer stopped.
    Testing call once, call last on destroy.
        Counter initialised with 0
        Tick #1
        Tick #2
        Tick #3
        Tick #4
        Tick #5
        Counter finished at 5
        Timer with optional cb done.
    Testing call once, call last on cancel.
        Counter initialised with 0
        Tick #1
        Tick #2
        Tick #3
        Tick #4
        Tick #5
        Counter finished at 5
        Timer with optional cb done.
    Testing with seconds.
        Tick #1
        Tick #2
        Tick #3
        Tick #4
        Tick #5
        Timer with seconds done.
    Testing reschedule.
        Tick #1
        Rescheduled to NOW
        Tick #2
        Rescheduled to really quick
        Tick #3
        Tick #4
        Rescheduled for a second
        Tick #5
        Timer rescheduling done.
    Testing finished.

---
