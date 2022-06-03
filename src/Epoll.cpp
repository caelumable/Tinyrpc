#include "Epoll.h"

#include <memory>
#include <unistd.h>

#include <cstring>

#include "Channel.h"
#include "util.h"

#define MAX_EVENTS 1000


Epoll::Epoll() {
  epfd_ = epoll_create1(0);
  ErrorIf(epfd_ == -1, "epoll create error");
  events_ = new epoll_event[MAX_EVENTS];
  memset(events_, 0, sizeof(*events_) * MAX_EVENTS);
}

Epoll::~Epoll() {
  if (epfd_ != -1) {
    close(epfd_);
    epfd_ = -1;
  }
  delete[] events_;
}

std::vector<Channel *> Epoll::Poll(int timeout) {
  std::vector<Channel *> active_channels;
  int nfds = epoll_wait(epfd_, events_, MAX_EVENTS, timeout);
  ErrorIf(nfds == -1, "epoll wait error");
  
  #ifdef debug
  if (-1 == nfds) {
    if (errno != EINTR) {
      exit(0);
    }
  }
  #endif 



  for (int i = 0; i < nfds; ++i) {
    // Channel *ch = (Channel *)events_[i].data.ptr;
    //because we put a plain pointer in ev.data.ptr, so we can't use unique ptr to store this pointer
    // we have to use shared_ptr to get this pointer
    //and this situlation explains that: the smart pointers are not perfect subsitution for plain pointer
    Channel *ch = (Channel*)events_[i].data.ptr;
    ch->SetReadyEvents(events_[i].events);
    active_channels.push_back(ch);
  }
  return active_channels;
}

void Epoll::UpdateChannel(std::weak_ptr<Channel> ch) {
  assert(!ch.expired());
  auto ch_s = ch.lock();
  int fd = ch_s->GetFd();
  struct epoll_event ev {};
  //here we have to give the row pointer, or we had to give the &shared_ptr<Channel>, which is not a good idea to write in this way
  ev.data.ptr = ch_s.get();
  ev.events = ch_s->GetListenEvents();
  if (!ch_s->GetInEpoll()) {
    ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_ADD, fd, &ev) == -1, "epoll add error");
    ch_s->SetInEpoll();
  } else {
    ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_MOD, fd, &ev) == -1,
            "epoll modify error");
  }
}

void Epoll::DeleteChannel(std::shared_ptr<Channel> ch) {
  int fd = ch->GetFd();
  ErrorIf(epoll_ctl(epfd_, EPOLL_CTL_DEL, fd, nullptr) == -1,
          "epoll delete error");
  ch->SetInEpoll(false);
}
