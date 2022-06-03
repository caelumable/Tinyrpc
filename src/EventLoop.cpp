#include "EventLoop.h"

#include <memory>
#include <vector>

#include "Channel.h"
#include "Epoll.h"

EventLoop::EventLoop() { epoll_ = new Epoll(); }

EventLoop::~EventLoop() { delete epoll_; }

void EventLoop::Loop() {
  while (!quit_) {
    std::vector<Channel *> chs;
    chs = epoll_->Poll();
    for (auto &ch : chs) {
      ch->HandleEvent();
    }
  }
}

void EventLoop::UpdateChannel(std::weak_ptr<Channel> ch) { epoll_->UpdateChannel(ch); }
