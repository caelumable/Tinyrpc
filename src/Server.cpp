#include "Server.h"

#include <cstdint>
#include <memory>
#include <unistd.h>

#include <functional>

#include "Acceptor.h"
#include "Connection.h"
#include "EventLoop.h"
#include "Socket.h"
#include "ThreadPool.h"
#include <iostream>
#include "util.h"

// #define debug

Server::Server(const std::unique_ptr<EventLoop> &loop,uint16_t port) : main_reactor_(loop), acceptor_(nullptr), thread_pool_(nullptr) {
  acceptor_.reset(new Acceptor(main_reactor_,port));
  std::function<void(std::unique_ptr<Socket> &)> cb = std::bind(&Server::NewConnection, this, std::placeholders::_1);
  acceptor_->SetNewConnectionCallback(cb);

  // int size = static_cast<int>(std::thread::hardware_concurrency());
  int size=1;
  thread_pool_ = new ThreadPool(size);
  for (int i = 0; i < size; ++i) {
    sub_reactors_.push_back(std::make_unique<EventLoop>());
  }

  for (int i = 0; i < size; ++i) {
    //because the bind function usually copied by value,so we need to use cref or ref to maks it copy by refference
    std::function<void()> sub_loop = std::bind(&EventLoop::Loop, std::cref(sub_reactors_[i]));
    thread_pool_->Add(std::move(sub_loop));
  }
}

Server::~Server() {
  // delete acceptor_;
  delete thread_pool_;
}

void Server::NewConnection(std::unique_ptr<Socket> &sock) {
  int temFd = sock->GetFd();
  ErrorIf(temFd == -1, "new connection error");
  uint64_t random = temFd % sub_reactors_.size();
 
  // auto conn =  std::make_shared<Connection>(sub_reactors_[random], std::move(sock));
  //here we are not use on_connnet_callback,it will only be used by epoll,and at that time
  //the type of  connection 
  std::unique_ptr<Connection> conn(new Connection(sub_reactors_[random],std::move(sock)));
  std::function<void(const unique_ptr<Socket> &)> cb = std::bind(&Server::DeleteConnection, this, std::placeholders::_1);
  conn->SetDeleteConnectionCallback(cb);
  conn->SetOnConnectCallback(on_connect_callback_);
  connections_[temFd] = std::move(conn);
}

void Server::DeleteConnection(const std::unique_ptr<Socket> &sock) {
  int sockfd = sock->GetFd();
  auto it = connections_.find(sockfd);
  if (it != connections_.end()) {
    //try if there could be weak
    std::shared_ptr<Connection> conn = connections_[sockfd];
    connections_.erase(sockfd);
    // delete conn;
    // conn = nullptr;
  }
}

void Server::OnConnect(std::function<void(std::shared_ptr<Connection>)> fn) { on_connect_callback_ = std::move(fn); }
