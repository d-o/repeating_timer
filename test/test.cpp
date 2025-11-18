/*
* Copyright (c) 2025 Dean Sellers (dean@sellers.id.au)
* SPDX-License-Identifier: MIT
*/

#include "repeatable_timer.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main() {

    // Test auto destruction
    {
        asio::io_context io;
        std::thread t;
        {
            std::cout << "Testing auto destruction.\n";
            // Create a timer that prints a counter every 10 millis.
            auto timer = RepeatingTimer<int>::create(
                io,
                [](int& counter) {
                    std::cout << "\tTick #" << ++counter << '\n';
                },
                std::chrono::milliseconds(10),
                std::make_shared<int>(0)  // initial counter value
            );

            // Run the io_context in its own thread
            t = std::thread ([&io]{ io.run(); });

            // Let it tick 5 times.
            std::this_thread::sleep_for(std::chrono::milliseconds(55));

        }
        // When the timer goes out of scope it stops automatically.
        t.join();
        std::cout << "\tTimer destroyed." << std::endl;
    }

    // Test cancel
    {
        std::cout << "Testing cancelling.\n";
        asio::io_context io;
        // Create a timer that prints a counter every 10 millis.
        auto timer = RepeatingTimer<int>::create(
            io,
            [](int& counter) {
                std::cout << "\tTick #" << ++counter << '\n';
            },
            std::chrono::milliseconds(10),
            std::make_shared<int>(0)  // initial counter value
        );

        // Run the io_context in its own thread
        std::thread io_thread([&io]{ io.run(); });

        // Let it tick 5 times.
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        // Manually cancel.
        timer->cancel();  // stop the timer

        io_thread.join();
        std::cout << "\tTimer stopped." << std::endl;
    }

    // Test run first cb, run last cb
    {
        std::cout << "Testing call once, call last on destroy.\n";
        asio::io_context io;
        // Create a timer that prints a counter 10 millis.
        auto timer = RepeatingTimer<int>::create(
            io,
            [](int& counter) {
                std::cout << "\tTick #" << ++counter << '\n';
            },
            std::chrono::milliseconds(10),
            std::make_shared<int>(0),  // initial counter value
            [](int& counter) {
                std::cout << "\tCounter initialised with " << counter << '\n';
            },
                [](int& counter) {
                std::cout << "\tCounter finished at " << counter << '\n';
            }
        );

        // Run the io_context in its own thread
        std::thread io_thread([&io]{ io.run(); });

        // Let it tick 5 times.
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        // When the timer goes out of scope it stops automatically.
        timer.reset();  // stop the timer

        io_thread.join();
        std::cout << "\tTimer with optional cb done." << std::endl;
    }

    // Test run first cb, run last cb, when cancelled
    {
        std::cout << "Testing call once, call last on cancel.\n";
        asio::io_context io;
        // Create a timer that prints a counter 10 millis.
        auto timer = RepeatingTimer<int>::create(
            io,
            [](int& counter) {
                std::cout << "\tTick #" << ++counter << '\n';
            },
            std::chrono::milliseconds(10),
            std::make_shared<int>(0),  // initial counter value
            [](int& counter) {
                std::cout << "\tCounter initialised with " << counter << '\n';
            },
                [](int& counter) {
                std::cout << "\tCounter finished at " << counter << '\n';
            }
        );

        // Run the io_context in its own thread
        std::thread io_thread([&io]{ io.run(); });

        // Let it tick 5 times.
        std::this_thread::sleep_for(std::chrono::milliseconds(55));
        timer->cancel();  // stop the timer

        io_thread.join();
        std::cout << "\tTimer with optional cb done." << std::endl;
    }


    // Test seconds overload
    {
        std::cout << "Testing with seconds.\n";
        asio::io_context io;
        // Create a timer that prints a counter every second.
        auto timer = RepeatingTimer<int>::create(
            io,
            [](int& counter) {
                std::cout << "\tTick #" << ++counter << '\n';
            },
            std::chrono::seconds(1),
            std::make_shared<int>(0)  // initial counter value
        );

        // Run the io_context in its own thread
        std::thread io_thread([&io]{ io.run(); });

        // Let it tick 5 times.
        std::this_thread::sleep_for(std::chrono::seconds(5));
        // When the timer goes out of scope it stops automatically.
        timer.reset();  // stop the timer

        io_thread.join();
        std::cout << "\tTimer with seconds done." << std::endl;
    }

    // Test reschedule
    {
        std::cout << "Testing reschedule.\n";
        asio::io_context io;
        // Create a timer that prints a counter every second.
        auto timer = RepeatingTimer<int>::create(
            io,
            [](int& counter) {
                std::cout << "\tTick #" << ++counter  << '\n';
            },
            std::chrono::seconds(1),
            std::make_shared<int>(0)  // initial counter value
        );

        // Run the io_context in its own thread
        std::thread io_thread([&io]{ io.run(); });

        // Let it tick once.
        std::this_thread::sleep_for(std::chrono::milliseconds(1100));
        std::cout << "\tRescheduled to NOW\n";
        timer->reschedule(std::chrono::milliseconds(0));
        // Wait a real little bit
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        std::cout << "\tRescheduled to really quick\n";
        timer->reschedule(std::chrono::milliseconds(10), true);
        // Wait for 2 more ticks
        std::this_thread::sleep_for(std::chrono::milliseconds(25));
        std::cout << "\tRescheduled for a second\n";
        timer->reschedule(std::chrono::seconds(1), true);
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        // When the timer goes out of scope it stops automatically.
        timer.reset();  // stop the timer

        io_thread.join();
        std::cout << "\tTimer rescheduling done." << std::endl;
    }

    std::cout << "Testing finished.\n";
}
