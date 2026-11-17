#include "util.h"

#include <unistd.h>

#ifdef __linux__

#include <sys/prctl.h>

namespace pyabc
{

void kill_on_parent_death(int sig)
{
    // kill process if parent dies
    prctl(PR_SET_PDEATHSIG, sig);

    // the parent might have died before calling prctl
    // in that case, it would be adopted by init, whose pid is 1
    if (getppid() == 1)
    {
        raise(sig);
    }
}

} // namespace pyabc

#elif defined(__APPLE__)

#include <thread>

#include <cassert>

#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>

namespace pyabc
{

void kill_on_parent_death(int sig)
{
  const int ppid = getppid();

  std::thread monitor_thread([ppid, sig](){

    struct kevent change;
    EV_SET(&change, ppid, EVFILT_PROC, EV_ADD, NOTE_EXIT, 0, nullptr);

    int kq = kqueue();
    if( kq < 0 )
    {
      return;
    }

    struct kevent event;
    struct timespec ts = {0, 0};

    // start listening, we are guaranteed to receive a notification if ppid is dead
    int rc = retry_eintr([&](){
       return kevent(kq, &change, 1, &event, 1, &ts);
    });

    // however, if ppid died before the call to kevent, ppid might not be the pid of the parent
    // in that case, the process it would be adopted by init, whose pid is 1
    if( getppid() == 1 )
    {
      raise(sig);
      return;
    }

    auto parent_exited = [&](int n){
      return n > 0 &&
        !(event.flags & EV_ERROR) &&
        event.filter == EVFILT_PROC &&
        event.ident == static_cast<uintptr_t>(ppid) &&
        (event.fflags & NOTE_EXIT);
    };

    // This thread also runs in children that only fork() in order to exec(), e.g. Python's
    // subprocess. Those close all inherited file descriptors before exec(), including kq,
    // which makes kevent() fail with EBADF. Only a real NOTE_EXIT for the parent may raise
    // the signal; on any other failure stop monitoring quietly, otherwise the child is
    // killed by its own SIGQUIT handler (_exit(1)) before it gets to exec().
    while( !parent_exited(rc) )
    {
      if( rc < 0 || (rc > 0 && (event.flags & EV_ERROR)) )
      {
        return;
      }

      // now block on kevent until the the parent process dies
      rc = retry_eintr([&](){
         return kevent(kq, nullptr, 0, &event, 1, nullptr);
      });
    }

    raise(sig);
  });

  monitor_thread.detach();
}

} // namespace pyabc

#else // neither linux or OS X

namespace pyabc
{

void kill_on_parent_death(int sig)
{
}

} // namespace pyabc

#endif // #ifdef __APPLE__
