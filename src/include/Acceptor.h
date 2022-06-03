#pragma once
#include "Macros.h"

#include <functional>
#include <future>
#include <memory>

class EventLoop;
class Socket;
class Channel;
class Acceptor {
 public:
  explicit Acceptor(const std::unique_ptr<EventLoop> &loop,uint16_t port=1234);
  ~Acceptor();

  DISALLOW_COPY_AND_MOVE(Acceptor);

  void AcceptConnection();
  void SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket> &)> const &callback);

 private:
  const std::unique_ptr<EventLoop> &loop_;
  std::unique_ptr<Socket> sock_;
  std::shared_ptr<Channel> channel_;
  std::function<void(std::unique_ptr<Socket> &)> new_connection_callback_;
};
