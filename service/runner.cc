
#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>

#include <iostream>
#include <utility>

#include "jml/arch/futex.h"
#include "jml/arch/timers.h"
#include "jml/utils/file_functions.h"

#include "message_loop.h"
#include "sink.h"

#include "runner.h"

using namespace std;
using namespace Datacratic;


namespace {

struct ChildStatus
{
    bool running;
    union {
        int pid;
        int status;
    };
};

tuple<int, int>
CreateStdPipe(bool forWriting)
{
    int fds[2];
    int rc = pipe(fds);
    if (rc == -1) {
        throw ML::Exception(errno, "CreateStdPipe pipe2");
    }

    if (forWriting) {
        return tuple<int, int>(fds[1], fds[0]);
    }
    else {
        return tuple<int, int>(fds[0], fds[1]);
    }
}

struct ChildFds {
    ChildFds()
        : stdIn(::fileno(stdin)),
          stdOut(::fileno(stdout)),
          stdErr(::fileno(stderr)),
          statusFd(-1)
    {
    }

    /* child api */
    void closeRemainingFds()
    {
        struct rlimit limits;
        ::getrlimit(RLIMIT_NOFILE, &limits);

        for (int fd = 0; fd < limits.rlim_cur; fd++) {
            if (fd != STDIN_FILENO
                && fd != STDOUT_FILENO && fd != STDERR_FILENO
                && fd != statusFd) {
                ::close(fd);
            }
        }
    }

    void dupToStdStreams()
    {
        auto dupToStdStream = [&] (int oldFd, int newFd) {
            if (oldFd != newFd) {
                int rc = ::dup2(oldFd, newFd);
                if (rc == -1) {
                    throw ML::Exception(errno,
                                        "ChildFds::dupToStdStream dup2");
                }
            }
        };
        dupToStdStream(stdIn, STDIN_FILENO);
        dupToStdStream(stdOut, STDOUT_FILENO);
        dupToStdStream(stdErr, STDERR_FILENO);
    }

    /* parent & child api */
    void close()
    {
        auto closeIfNotEqual = [&] (int & fd, int notValue) {
            if (fd != notValue) {
                ::close(stdIn);
            }
        };
        closeIfNotEqual(stdIn, STDIN_FILENO);
        closeIfNotEqual(stdOut, STDOUT_FILENO);
        closeIfNotEqual(stdErr, STDERR_FILENO);
        closeIfNotEqual(statusFd, -1);
    }

    int stdIn;
    int stdOut;
    int stdErr;
    int statusFd;
};

void
RunWrapper(const vector<string> & command, ChildFds & fds)
{
    fds.dupToStdStreams();
    fds.closeRemainingFds();

    int childPid = fork();
    if (childPid == -1) {
        throw ML::Exception(errno, "RunWrapper fork");
    }
    else if (childPid == 0) {
        ::close(fds.statusFd);
        size_t len = command.size();
        char * argv[len + 1];
        for (int i = 0; i < len; i++) {
            argv[i] = strdup(command[i].c_str());
        }
        argv[len] = NULL;
        execv(command[0].c_str(), argv);
        throw ML::Exception("The Alpha became the Omega.");
    }
    else {
        // FILE * terminal = ::fopen("/dev/tty", "a");

        // ::fprintf(terminal, "wrapper: real child pid: %d\n", childPid);
        ChildStatus status;

        status.running = true;
        status.pid = childPid;
        if (::write(fds.statusFd, &status, sizeof(status)) == -1) {
            throw ML::Exception(errno, "RunWrapper write");
        }

        // ::fprintf(terminal, "wrapper: waiting child...\n");

        waitpid(childPid, &status.status, 0);

        // ::fprintf(terminal, "wrapper: child terminated\n");

        status.running = false;
        if (::write(fds.statusFd, &status, sizeof(status)) == -1) {
            throw ML::Exception(errno, "RunWrapper write");
        }

        fds.close();

        exit(0);
    }
}

} // namespace


/* ASYNCRUNNER */

AsyncRunner::
AsyncRunner()
    : running_(false)
{
    Epoller::init(4);

    handleEvent = bind(&AsyncRunner::handleEpollEvent, this,
                       placeholders::_1);
}

AsyncRunner::
~AsyncRunner()
{
    waitTermination();
}

bool
AsyncRunner::
handleEpollEvent(const struct epoll_event & event)
{
    if (task_) {
        Task & task = *task_;
        if (event.data.ptr == &task.statusFd) {
            // fprintf(stderr, "parent: handle child status input\n");
            handleChildStatus(event, task.statusFd, task);
        }
        else if (event.data.ptr == &task.stdOutFd) {
            // fprintf(stderr, "parent: handle child output from stdout\n");
            handleOutputStatus(event, task.stdOutFd, *stdOutSink_);
        }
        else if (event.data.ptr == &task.stdErrFd) {
            // fprintf(stderr, "parent: handle child output from stderr\n");
            handleOutputStatus(event, task.stdErrFd, *stdErrSink_);
        }
        else {
            throw ML::Exception("this should never occur");
        }
    }
    else {
        throw ML::Exception("received event for ghost task");
    }

    return false;
}

void
AsyncRunner::
handleChildStatus(const struct epoll_event & event, int statusFd, Task & task)
{
    ChildStatus status;

    // cerr << "handleChildStatus\n";
    ssize_t s = ::read(statusFd, &status, sizeof(status));
    if (s == -1) {
        throw ML::Exception(errno, "AsyncRunner::handleChildStatus read");
    }

    if (status.running) {
        task.childPid = status.pid;
        restartFdOneShot(statusFd, event.data.ptr);
    }
    else {
        RunResult result(status.status);
        task.postTerminate(*this, result);
        running_ = false;
        if (stdInSink_) {
            stdInSink_->requestClose();
        }
        else {
            task_.reset(nullptr);
        }
        ML::futex_wake(running_);
    }
}

void
AsyncRunner::
handleOutputStatus(const struct epoll_event & event,
                   int outputFd, InputSink & sink)
{
    char buffer[4096];
    bool closedFd(false);
    string data;

    if ((event.events & EPOLLIN) != 0) {
        while (1) {
            ssize_t len = ::read(outputFd, buffer, sizeof(buffer));
            // cerr << "returned len: " << len << endl;
            if (len < 0) {
                // perror("  len -1");
                if (errno == EWOULDBLOCK) {
                    break;
                }
                else if (errno == EBADF) {
                    closedFd = true;
                    break;
                }
                else {
                    throw ML::Exception(errno,
                                        "AsyncRunner::handleOutputStatus read");
                }
            }
            else if (len == 0) {
                closedFd = true;
                break;
            }
            else if (len > 0) {
                data.append(buffer, len);
            }
        }

        if (data.size() > 0) {
            // cerr << "sending child output to output handler\n";
            sink.notifyReceived(move(data));
        }
        else {
            cerr << "ignoring child output due to size == 0\n";
        }
    }

    // cerr << "closedFd: " << closedFd << endl;
    if (closedFd || (event.events & EPOLLHUP) == 0) {
        sink.notifyClosed();
    }
    else {
        JML_TRACE_EXCEPTIONS(false);
        try {
            restartFdOneShot(outputFd, event.data.ptr);
        }
        catch (const ML::Exception & exc) {
            sink.notifyClosed();
        }
    }
}

OutputSink &
AsyncRunner::
getStdInSink()
{
    if (running_)
        throw ML::Exception("already running");
    if (stdInSink_)
        throw ML::Exception("stdin sink already set");

    auto onClosed = [&] (const OutputSink & closedSink) {
        this->onStdInClosed(closedSink);
    };
    stdInSink_.reset(new AsyncFdOutputSink(onClosed));

    return *stdInSink_;
}

void
AsyncRunner::
run(const vector<string> & command,
    const OnTerminate & onTerminate,
    const shared_ptr<InputSink> & stdOutSink,
    const shared_ptr<InputSink> & stdErrSink)
{
    if (task_)
        throw ML::Exception("already running");

    running_ = true;
    task_.reset(new Task());
    Task & task = *task_;

    task.onTerminate = onTerminate;

    ChildFds childFds;
    tie(task.statusFd, childFds.statusFd) = CreateStdPipe(false);

    if (stdInSink_) {
        tie(task.stdInFd, childFds.stdIn) = CreateStdPipe(true);
    }
    if (stdOutSink) {
        stdOutSink_ = stdOutSink;
        tie(task.stdOutFd, childFds.stdOut) = CreateStdPipe(false);
    }
    if (stdErrSink) {
        stdErrSink_ = stdErrSink;
        tie(task.stdErrFd, childFds.stdErr) = CreateStdPipe(false);
    }

    task.wrapperPid = fork();
    if (task.wrapperPid == -1) {
        throw ML::Exception(errno, "AsyncRunner::run fork");
    }
    else if (task.wrapperPid == 0) {
        RunWrapper(command, childFds);
    }
    else {
        ML::set_file_flag(task.statusFd, O_NONBLOCK);
        if (stdInSink_) {
            ML::set_file_flag(task.stdInFd, O_NONBLOCK);
            stdInSink_->init(task.stdInFd);
            parent_->addSource("stdInSink", stdInSink_);
        }
        addFdOneShot(task.statusFd, &task.statusFd);
        if (stdOutSink) {
            ML::set_file_flag(task.stdOutFd, O_NONBLOCK);
            addFdOneShot(task.stdOutFd, &task.stdOutFd);
        }
        if (stdErrSink) {
            ML::set_file_flag(task.stdErrFd, O_NONBLOCK);
            addFdOneShot(task.stdErrFd, &task.stdErrFd);
        }

        childFds.close();
    }
}

void
AsyncRunner::
kill(int signum)
{
    if (!task_)
        throw ML::Exception("subprocess has already terminated");

    ::kill(task_->childPid, signum);
    waitTermination();
}

void
AsyncRunner::
waitTermination()
{
    while (task_ && task_->stdInFd != -1) {
        int oldVal = task_->stdInFd;
        ML::futex_wait(task_->stdInFd, oldVal);
    }

    while (running_) {
        ML::futex_wait(running_, true);
    }
}

void
AsyncRunner::
onStdInClosed(const OutputSink & sink)
{
    if (task_) {
        task_->stdInFd = -1;
        ML::futex_wake(task_->stdInFd);
        task_.reset(nullptr);
    }
    else {
        if (stdInSink_ && !stdInSink_->closed()) {
            cerr << "task dead but stdin sink not closed\n";
        }
    }

    parent_->removeSource(stdInSink_.get());
}


/* ASYNCRUNNER::TASK */

void
AsyncRunner::
Task::
postTerminate(AsyncRunner & runner, const RunResult & runResult)
{
    // cerr << "postTerminate\n";

    waitpid(wrapperPid, NULL, 0);

    auto unregisterFd = [&] (int & fd) {
        if (fd > -1) {
            runner.removeFd(fd);
            ::close(fd);
            fd = -1;
        }
    };
    unregisterFd(stdOutFd);
    unregisterFd(stdErrFd);
    unregisterFd(statusFd);

    if (onTerminate) {
        onTerminate(runResult);
    }
}


/* ASYNCRUNNER::RUNRESULT */

void
AsyncRunner::
RunResult::
updateFromStatus(int status)
{
    if (WIFEXITED(status)) {
        signaled = false;
        returnCode = WEXITSTATUS(status);
    }
    else if (WIFSIGNALED(status)) {
        signaled = true;
        signum = WTERMSIG(status);
    }
}


/* EXECUTE */

AsyncRunner::RunResult
Datacratic::
Execute(const vector<string> & command,
        const shared_ptr<InputSink> & stdOutSink,
        const shared_ptr<InputSink> & stdErrSink,
        const string & stdInData)
{
    AsyncRunner::RunResult result;
    auto onTerminate = [&](const AsyncRunner::RunResult & runResult) {
        result = runResult;
    };

    MessageLoop loop;
    AsyncRunner runner;

    loop.addSource("runner", runner);
    loop.start();

    if (stdInData.size() > 0) {
        auto & sink = runner.getStdInSink();
        runner.run(command, onTerminate, stdOutSink, stdErrSink);
        sink.write(string(stdInData));
        sink.requestClose();
    }
    else {
        runner.run(command, onTerminate, stdOutSink, stdErrSink);
    }

    runner.waitTermination();

    return result;
}