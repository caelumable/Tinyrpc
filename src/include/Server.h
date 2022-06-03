#pragma once
#include "Macros.h"

#include <functional>
#include <future>
#include <map>
#include <memory>
#include <vector>
#include <cassert>
class EventLoop;
class Socket;
class Acceptor;
class Connection;
class ThreadPool;
class Server {
 private:
  const std::unique_ptr<EventLoop> &main_reactor_;
  std::unique_ptr<Acceptor> acceptor_;
  std::map<int, std::shared_ptr<Connection> > connections_;
  std::vector<std::unique_ptr<EventLoop>> sub_reactors_;
  ThreadPool *thread_pool_;
  std::function<void(std::shared_ptr<Connection>)> on_connect_callback_;

 public:
  explicit Server(const std::unique_ptr<EventLoop> &loop,uint16_t port=1234);
  ~Server();

  DISALLOW_COPY_AND_MOVE(Server);

  void NewConnection(std::unique_ptr<Socket> &sock);
  void DeleteConnection(const std::unique_ptr<Socket> &sock);
  void OnConnect(std::function<void(std::shared_ptr<Connection>)> fn);
};
