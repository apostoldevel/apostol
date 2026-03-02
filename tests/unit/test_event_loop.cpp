#include <catch2/catch_test_macros.hpp>

#include "apostol/event_loop.hpp"

#include <atomic>
#include <chrono>
#include <csignal>
#include <unistd.h>

using namespace apostol;
using namespace std::chrono_literals;

// ─── Timer tests ──────────────────────────────────────────────────────────────

TEST_CASE("EventLoop: one-shot timer fires once and stops loop", "[event_loop]")
{
    EventLoop loop;
    int fired = 0;

    loop.add_timer(20ms, [&] {
        ++fired;
        loop.stop();
    }, /*repeat=*/false);

    loop.run();

    REQUIRE(fired == 1);
}

TEST_CASE("EventLoop: repeating timer fires multiple times", "[event_loop]")
{
    EventLoop loop;
    int count = 0;

    loop.add_timer(15ms, [&] {
        ++count;
        if (count >= 3)
            loop.stop();
    });

    loop.run();

    REQUIRE(count == 3);
}

TEST_CASE("EventLoop: timer can be cancelled before firing", "[event_loop]")
{
    EventLoop loop;
    int fired = 0;

    // Short timer to cancel, long timer to end the loop
    auto cancel_id = loop.add_timer(10ms, [&] { ++fired; }, /*repeat=*/false);

    loop.add_timer(50ms, [&] { loop.stop(); }, /*repeat=*/false);

    loop.cancel_timer(cancel_id);
    loop.run();

    REQUIRE(fired == 0);
}

TEST_CASE("EventLoop: cancelling timer from within callback is safe", "[event_loop]")
{
    EventLoop loop;
    int fired = 0;
    EventLoop::TimerId tid = EventLoop::kInvalidTimer;

    tid = loop.add_timer(15ms, [&] {
        ++fired;
        loop.cancel_timer(tid); // cancel self
        loop.stop();
    });

    loop.run();
    REQUIRE(fired == 1);
}

TEST_CASE("EventLoop: multiple independent timers fire in order", "[event_loop]")
{
    EventLoop loop;
    std::vector<int> order;

    loop.add_timer(10ms, [&] { order.push_back(1); }, false);
    loop.add_timer(30ms, [&] { order.push_back(2); loop.stop(); }, false);

    loop.run();

    REQUIRE(order.size() == 2);
    REQUIRE(order[0] == 1);
    REQUIRE(order[1] == 2);
}

// ─── I/O tests ────────────────────────────────────────────────────────────────

TEST_CASE("EventLoop: add_io fires callback when pipe is readable", "[event_loop]")
{
    int pipefd[2];
    REQUIRE(::pipe(pipefd) == 0);

    EventLoop loop;
    bool called = false;

    loop.add_io(pipefd[0], EPOLLIN, [&](uint32_t) {
        called = true;
        loop.stop();
    });

    // Write to pipe to make it readable, then run
    (void)::write(pipefd[1], "x", 1);
    loop.run();

    REQUIRE(called);

    ::close(pipefd[0]);
    ::close(pipefd[1]);
}

TEST_CASE("EventLoop: remove_io stops receiving callbacks", "[event_loop]")
{
    int pipefd[2];
    REQUIRE(::pipe(pipefd) == 0);

    EventLoop loop;
    int count = 0;

    // Callback reads the byte (level-triggered: must drain the buffer)
    loop.add_io(pipefd[0], EPOLLIN, [&](uint32_t) {
        char buf;
        ::read(pipefd[0], &buf, 1);
        ++count;
    });

    // Timer removes the handler, then writes again and stops
    loop.add_timer(30ms, [&] {
        loop.remove_io(pipefd[0]);
        (void)::write(pipefd[1], "y", 1); // second write — should NOT trigger callback
        loop.stop();
    }, false);

    (void)::write(pipefd[1], "x", 1); // first write — triggers callback once
    loop.run();

    // First byte was consumed by the callback, second write happened after remove_io
    REQUIRE(count == 1);

    ::close(pipefd[0]);
    ::close(pipefd[1]);
}

// ─── Signal tests ────────────────────────────────────────────────────────────

TEST_CASE("EventLoop: add_signal receives SIGUSR1 sent from self", "[event_loop]")
{
    EventLoop loop;
    bool received = false;

    loop.add_signal(SIGUSR1, [&](const signalfd_siginfo&) {
        received = true;
        loop.stop();
    });

    // Send signal to self via timer
    loop.add_timer(10ms, [&] { ::kill(::getpid(), SIGUSR1); }, false);

    loop.run();

    // Unblock SIGUSR1 after test (restore state)
    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask, SIGUSR1);
    ::sigprocmask(SIG_UNBLOCK, &mask, nullptr);

    REQUIRE(received);
}

// ─── stop() ──────────────────────────────────────────────────────────────────

TEST_CASE("EventLoop: stop() called from timer terminates loop", "[event_loop]")
{
    EventLoop loop;

    loop.add_timer(10ms, [&] { loop.stop(); }, false);
    loop.run(); // must return

    REQUIRE_FALSE(loop.running());
}
