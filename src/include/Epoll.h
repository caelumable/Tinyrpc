#pragma once
#include "Macros.h"

#include <memory>
#include <vector>

#ifdef OS_LINUX
#include <sys/epoll.h>
#endif

class Channel;
class Epoll {
 public:
  Epoll();
  ~Epoll();

  DISALLOW_COPY_AND_MOVE(Epoll);

  void UpdateChannel(std::weak_ptr<Channel> ch);
  void DeleteChannel(std::shared_ptr<Channel> ch);

  std::vector<Channel*> Poll(int timeout = -1);

 private:
  int epfd_{1};
  struct epoll_event *events_{nullptr};
};
