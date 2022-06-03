
#include "include/Acceptor.h"

#include <future>
#include <memory>
#include <utility>

#include "include/Channel.h"

#include "include/Socket.h"

#include <iostream>

// #define debug

Acceptor::Acceptor(const std::unique_ptr<EventLoop> &loop,uint16_t port) : loop_(loop), sock_(nullptr), channel_(nullptr) {
  sock_ = std::make_unique<Socket>();
  std::unique_ptr<InetAddress> addr (new InetAddress("127.0.0.1", port));
  sock_->Bind(addr);
  sock_->Listen();
  channel_.reset(new Channel(loop_, sock_->GetFd()));
  std::function<void()> cb = std::bind(&Acceptor::AcceptConnection, this);
  channel_->SetReadCallback(cb);
  //enableRead is going to put the channel to epoll,so the system could get event
  //the EnableRead call the function of updateChannel
  channel_->EnableRead();
}

Acceptor::~Acceptor() {
}

void Acceptor::AcceptConnection() {
  std::unique_ptr<InetAddress> clnt_addr ( new InetAddress());
  std::unique_ptr<Socket> clnt_sock = std::make_unique<Socket>(sock_->Accept(clnt_addr));
  printf("new client fd %d! IP: %s Port: %d\n", clnt_sock->GetFd(), clnt_addr->GetIp(), clnt_addr->GetPort());
  clnt_sock->SetNonBlocking();  // 新接受到的连接设置为非阻塞式
  new_connection_callback_(clnt_sock);
}

void Acceptor::SetNewConnectionCallback(std::function<void(std::unique_ptr<Socket> &)> const &callback) {
  new_connection_callback_ = callback;
}
