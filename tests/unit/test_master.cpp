// tests/unit/test_master.cpp
//
// Tests for the master-process shutdown mechanics:
//
//   Test 1 — SIGKILL escalation:
//     Fork a stubborn child that ignores SIGTERM. Arm a 1-second one-shot
//     timer (mirrors kill_timeout_secs_ = 1). Verify that the timer fires,
//     SIGKILL is sent, the child is reaped, and the loop exits cleanly.
//
//   Test 2 — clean exit before kill timer:
//     Fork a cooperative child that exits immediately on SIGTERM. Arm the
//     same timer with a 5-second deadline. Verify that the SIGCHLD handler
//     cancels the timer before it fires and the child exited with code 0.
//
// Both tests replicate the exact EventLoop logic from Application::master_run()
// (SIGCHLD handler + kill escalation timer in fast_stop lambda).
//
// Fork safety notes:
//   • Catch2 inherits open fds AND signal handlers in the child process.
//     A race between fork() and signal(SIGTERM, ...) in the child causes
//     Catch2's SIGTERM handler to fire → failure report + test re-run.
//   • We use a self-pipe to synchronise: the child writes one byte once
//     it has installed its signal handlers; the parent reads that byte and
//     only then sends SIGTERM.  This eliminates the race entirely.
//   • The child redirects stdout/stderr to /dev/null before writing to the
//     pipe, so Catch2's output from the child does not contaminate the
//     parent's test report.
//
// Tags: [process][master]
//   Run with:  ./apostol_tests "[master]"

#include <catch2/catch_test_macros.hpp>

#include "apostol/event_loop.hpp"

#include <csignal>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace apostol;
using namespace std::chrono_literals;

// ─── helpers ──────────────────────────────────────────────────────────────────

namespace {

// Redirect child's stdout/stderr to /dev/null.
// Must be called BEFORE write()-ing to the sync pipe, so that any Catch2
// output already buffered in userspace streams does not appear on the terminal
// when the child's exit path flushes buffers.
static void child_silence_output()
{
    int null_fd = ::open("/dev/null", O_WRONLY);
    if (null_fd >= 0)
    {
        ::dup2(null_fd, STDOUT_FILENO);
        ::dup2(null_fd, STDERR_FILENO);
        ::close(null_fd);
    }
}

// Unblock ALL signals (sets empty mask).
// Call AFTER installing the desired disposition so no signal is deliverable
// with an unexpected handler.
static void child_unblock_all_signals()
{
    sigset_t empty;
    sigemptyset(&empty);
    ::sigprocmask(SIG_SETMASK, &empty, nullptr);
}

// SIGTERM handler for the cooperative child: exit cleanly.
static void sigterm_exit(int) { ::_exit(0); }

// Fork a child process with a self-pipe sync channel.
//
// @p setup_fn is called in the child (after output is silenced, signal
// handler installed, signals unblocked) to register the SIGTERM disposition
// and any other per-child setup. It must end with:
//   (void)::write(ready_fd, "R", 1); ::close(ready_fd);
//   for (;;) ::pause();
//   ::_exit(0);
//
// Returns {child_pid, parent_read_fd}.  The caller MUST ::read one byte from
// parent_read_fd and then ::close it before sending any signal to child_pid.
struct ForkResult { pid_t pid; int ready_fd; };

ForkResult fork_synced(std::function<void(int ready_fd)> child_fn)
{
    int pipe_fds[2];
    REQUIRE(::pipe2(pipe_fds, O_CLOEXEC) == 0);
    // pipe_fds[0] = read end (parent),  pipe_fds[1] = write end (child)

    pid_t child = ::fork();
    REQUIRE(child >= 0);

    if (child == 0)
    {
        ::close(pipe_fds[0]); // child does not need read end
        child_fn(pipe_fds[1]);
        // child_fn must not return
        ::_exit(0); // unreachable
    }

    // Parent: close write end and return read end + child PID
    ::close(pipe_fds[1]);
    return {child, pipe_fds[0]};
}

// Wait for the child to signal readiness via the sync pipe, then close it.
static void wait_for_ready(int ready_fd)
{
    char byte = 0;
    ::read(ready_fd, &byte, 1); // blocks until child writes "R"
    ::close(ready_fd);
}

} // namespace

// ─── Test 1: SIGKILL escalation ──────────────────────────────────────────────
//
// Scenario: stubborn worker ignores SIGTERM → 1-second kill timer fires
//           → SIGKILL sent → child reaped.
//
// EventLoop setup mirrors Application::master_run() fast_stop + SIGCHLD handler.
//
// Expected:
//   timer_fired       == true
//   child_reaped      == true
//   child_term_signal == SIGKILL (9)

TEST_CASE("master shutdown: SIGKILL escalation fires after timeout", "[process][master]")
{
    auto [child, ready_fd] = fork_synced([](int rfd) {
        child_silence_output();
        ::signal(SIGTERM, SIG_IGN);     // install BEFORE unblocking
        child_unblock_all_signals();
        (void)::write(rfd, "R", 1);           // notify parent: ready
        ::close(rfd);
        for (;;) ::pause();             // wait until SIGKILL
        ::_exit(0);                     // unreachable
    });

    wait_for_ready(ready_fd); // parent blocks here until child is ready

    // ── parent: EventLoop with 1-second SIGKILL escalation timer ─────────────

    bool timer_fired    = false;
    bool child_reaped   = false;
    int  child_term_sig = 0;

    EventLoop::TimerId kill_timer = EventLoop::kInvalidTimer;
    EventLoop loop;

    // SIGCHLD — reap child; cancel kill timer if it is still armed
    loop.add_signal(SIGCHLD, [&](const signalfd_siginfo&) {
        int status = 0;
        if (::waitpid(child, &status, WNOHANG) == child)
        {
            child_reaped = true;
            if (WIFSIGNALED(status))
                child_term_sig = WTERMSIG(status);
            if (kill_timer != EventLoop::kInvalidTimer)
            {
                loop.cancel_timer(kill_timer);
                kill_timer = EventLoop::kInvalidTimer;
            }
            loop.stop();
        }
    });

    // Send SIGTERM (child ignores it) and arm 1-second SIGKILL escalation timer
    ::kill(child, SIGTERM);
    kill_timer = loop.add_timer(1s,
        [&]() {
            timer_fired = true;
            kill_timer  = EventLoop::kInvalidTimer;
            if (::kill(child, 0) == 0) // child still alive?
                ::kill(child, SIGKILL);
            // SIGCHLD will reap the child and stop the loop
        },
        /*repeat=*/false);

    // Safety net — prevent infinite hang in CI
    loop.add_timer(5s, [&]{ loop.stop(); }, false);

    loop.run();

    // ── assertions ────────────────────────────────────────────────────────────
    CHECK(timer_fired);
    CHECK(child_reaped);
    CHECK(child_term_sig == SIGKILL);
}

// ─── Test 2: clean exit before kill timer ────────────────────────────────────
//
// Scenario: cooperative worker exits on SIGTERM → SIGCHLD cancels the 5-second
//           kill timer before it fires → master exits cleanly.
//
// Expected:
//   timer_fired      == false   (cancelled by SIGCHLD before it fired)
//   child_reaped     == true
//   child_exit_code  == 0

TEST_CASE("master shutdown: clean exit cancels kill timer", "[process][master]")
{
    auto [child, ready_fd] = fork_synced([](int rfd) {
        child_silence_output();
        ::signal(SIGTERM, sigterm_exit); // install BEFORE unblocking
        child_unblock_all_signals();
        (void)::write(rfd, "R", 1);            // notify parent: ready
        ::close(rfd);
        for (;;) ::pause();              // wait for SIGTERM → _exit(0)
        ::_exit(0);                      // unreachable
    });

    wait_for_ready(ready_fd);

    // ── parent: EventLoop with 5-second kill timer (must NOT fire) ────────────

    bool timer_fired    = false;
    bool child_reaped   = false;
    int  child_exit_code = -1;

    EventLoop::TimerId kill_timer = EventLoop::kInvalidTimer;
    EventLoop loop;

    // SIGCHLD — reap child; cancel the kill timer
    loop.add_signal(SIGCHLD, [&](const signalfd_siginfo&) {
        int status = 0;
        if (::waitpid(child, &status, WNOHANG) == child)
        {
            child_reaped = true;
            if (WIFEXITED(status))
                child_exit_code = WEXITSTATUS(status);
            if (kill_timer != EventLoop::kInvalidTimer)
            {
                loop.cancel_timer(kill_timer);
                kill_timer = EventLoop::kInvalidTimer;
            }
            loop.stop();
        }
    });

    // Send SIGTERM (child will exit); arm 5-second kill timer
    ::kill(child, SIGTERM);
    kill_timer = loop.add_timer(5s,
        [&]() {
            timer_fired = true; // must NOT reach here
            kill_timer  = EventLoop::kInvalidTimer;
            ::kill(child, SIGKILL);
            loop.stop();
        },
        /*repeat=*/false);

    // Safety net
    loop.add_timer(8s, [&]{ loop.stop(); }, false);

    loop.run();

    // ── assertions ────────────────────────────────────────────────────────────
    CHECK(!timer_fired);     // kill timer cancelled by SIGCHLD before it fired
    CHECK(child_reaped);
    CHECK(child_exit_code == 0);
}
