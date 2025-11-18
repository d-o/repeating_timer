/*
* Copyright (c) 2025 Dean Sellers (dean@sellers.id.au)
* SPDX-License-Identifier: MIT
*/

#include "repeatable_timer.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

/* Context for any timer */
struct timer_context {
    /* Shared by all instances */
    inline static std::mutex context_lock;

    /* Instance data */
    const int index;
    int counter;

    timer_context(int i, int c = 0) : index(i), counter(c) {}

    void on_timer() {
        /* Do some stuff ... */
        { /* Critical section */
            /* A shared resource is being accessed here - ie: cout */
            std::lock_guard<std::mutex> lock(context_lock);
            std::cout << "\tContext " <<  this->index << ":#" << ++this->counter << '\n';
        }
        /* Unlocked .. do other stuff*/
    }
};

int main() {

    constexpr static int NUM_TIMERS = 5;

    /* Create an empty vector of threads */
    std::vector<std::thread> threads;

    /* asio context */
    asio::io_context io;

    /* An application specific variable */
    std::atomic<size_t> my_counter(0);

    /* Now create a timer running in each thread in a new scope */
    {
        /* We need to a reference to each timer somewhere */
        std::vector<std::shared_ptr<RepeatingTimer<timer_context>>> timers;
        /* Create the timers */
        for(int i=1; i<=NUM_TIMERS; i++)
        {
            /* Create a timer that prints a counter every 1 milli. */
            auto timer = RepeatingTimer<timer_context>::create(
                io,
                [lambda_ctx = timer_context(i), &my_counter](timer_context& c) mutable { my_counter++; lambda_ctx.on_timer(); },
                std::chrono::milliseconds(1),
                nullptr  /* Empty context */
            );
            /* Save the timer in our vector in the outer scope */
            timers.push_back(timer);
        }
        /* Create a new thread for each timer
        This doesn't mean that each timer WILL run in a separate thread
        I am just trying to give asio the opportunity to do that */
        std::cout << "Create threads." << std::endl;
        for(size_t i=1; i<=NUM_TIMERS; i++) {
            threads.push_back(std::thread ([&io]{ io.run();}));
        }
        /* Let them all tick for a bit */
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    /* Once our timers are out of scope, they are stopped */
    for(auto& t : threads) {
        t.join();
    }
    std::cout << "Timers finished callback counter - " << my_counter << std::endl;
}
