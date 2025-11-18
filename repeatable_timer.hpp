/*
* Copyright (c) 2025 Dean Sellers (dean@sellers.id.au)
* SPDX-License-Identifier: MIT
*/

#pragma once

#include <asio.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <atomic>
#include <mutex>
#include <iostream>

/* A reusable, self‑rescheduling timer that carries a user‑supplied context.

  The callback signature is `void(Context&)`.
  A `std::mutex` protects the context when the `io_context` runs on several threads.
  See ./README.md for details
  ./test/test.cpp has a test usage with cmake to build repeating_timer_test.
*/
template <typename Context>
class RepeatingTimer
    : public std::enable_shared_from_this<RepeatingTimer<Context>>
{
public:
    using Callback = std::function<void(Context&)>;


    /// Create the timer, store the callback & context, then kick off the first tick.
    /// cb_once if it exists
    static std::shared_ptr<RepeatingTimer> create(
        asio::io_context& io,
        Callback cb,
        std::chrono::milliseconds period,
        std::shared_ptr<Context> ctx,
        Callback cb_once = nullptr,
        Callback cb_last = nullptr)
    {
        auto timer = std::shared_ptr<RepeatingTimer>(
            new RepeatingTimer(io, period, std::move(ctx)));

        timer->callback_ = std::move(cb);
        timer->callfirst_ = std::move(cb_once);
        timer->calllast_ = std::move(cb_last);

        // Initialise the timer's expiry to now
        timer->timer_.expires_after(std::chrono::steady_clock::duration(0));
        timer->schedule_next();          // start the loop
        return timer;
    }

    // Reschedule a running timer, can be once or persistent
    void reschedule(std::chrono::milliseconds newPeriod, bool saveNew = false)
    {
        std::lock_guard<std::mutex> l(mtx_);
        timer_.cancel();

        if (saveNew) {
            period_ = newPeriod;
        }
        // Set the expiry to now, ensures the new period is applied
        timer_.expires_after(std::chrono::steady_clock::duration(0));
        schedule_next(newPeriod);
    }

    // Reschedule with same period, restart really
    void reschedule() {
        reschedule(period_);
    }

    /// Stop the timer early (the destructor does the same).
    void cancel()
    {
        running_ = false;
        timer_.cancel();
        // Run the last call cb
        if (calllast_) {
            if(context_) std::lock_guard<std::mutex> lock(mtx_);
            calllast_(*context_);
            calllast_ = nullptr;
        }
    }

    ~RepeatingTimer() { cancel(); }

private:
    RepeatingTimer(asio::io_context& io,
                   std::chrono::milliseconds period,
                   std::shared_ptr<Context> ctx)
        : timer_(io),
          period_(period),
          running_(true),
          context_(std::move(ctx))
    {}

    // Deleted copy/move to avoid accidental misuse
    RepeatingTimer(const RepeatingTimer&) = delete;
    RepeatingTimer& operator=(const RepeatingTimer&) = delete;
    RepeatingTimer(RepeatingTimer&&) = delete;
    RepeatingTimer& operator=(RepeatingTimer&&) = delete;

    // Lock shared across all instances,
    inline static std::mutex mtx_;

    void schedule_next() {
        schedule_next(period_);
    }

    void schedule_next(std::chrono::milliseconds this_period)
    {
        // Don't do anything if we've been cancelled
        if (!running_)
            return;

        // Reset the timer and add a lambda to run when it expires
        // There is once case with `callfirst_` if it is true don't add the period
        // which ensures the timer will fire as soon as the async context schedules it
        if(callfirst_) {
            timer_.expires_at(timer_.expiry());
        }
        else {
            timer_.expires_at(timer_.expiry() + this_period);
        }
        // Use a weak pointer to pass a reference to the owning object into the lambda
        // inside it, if you can't lock the weak pointer then the object is no longer referenced
        std::weak_ptr<RepeatingTimer<Context>> wptr = this->shared_from_this();
        timer_.async_wait([wptr](const asio::error_code& ec)
        {
            if (ec == asio::error::operation_aborted)
                return;                    // cancelled
            if (ec)
            {
                std::cerr << "RepeatingTimer error: " << ec.message() << '\n';
                return;
            }
            // Make sure that the timer object is still referenced
            if(auto self = wptr.lock()) {
                // Guard the context against concurrent access, if a context is set
                {
                    if (self->context_) std::lock_guard<std::mutex> lock(self->mtx_);
                    // If callfirst_ is callable do it now ... then destroy it
                    // So .. call first and never again.
                    if (self->callfirst_) {
                        self->callfirst_(*self->context_);
                        self->callfirst_ = nullptr;
                    }
                    else if (self->callback_)
                        self->callback_(*self->context_);
                }
                // Reschedule only if still alive
                if (self->running_)
                    self->schedule_next();
            }
        });
    }

    asio::steady_timer timer_;
    std::chrono::milliseconds period_;
    std::atomic<bool> running_;
    std::shared_ptr<Context> context_;
    Callback callback_;
    Callback callfirst_;
    Callback calllast_;
};
