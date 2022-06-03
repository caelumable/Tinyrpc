#include "Connection.h"

#include <memory>
#include <unistd.h>

#include <cassert>
#include <cstring>
#include <iostream>
#include <utility>

#include "Channel.h"
#include "Serializer.hpp"
#include "Socket.h"
#include "util.h"
#include <iostream>
// #define debug

Connection::Connection(const std::unique_ptr<EventLoop> &loop, std::unique_ptr<Socket> sock) : loop_(loop), sock_(std::move(sock)) {
  if (loop_ != nullptr) {
    channel_.reset(new Channel(loop_, sock_->GetFd()));
    //here we put the client channel into epoll
    //this channel used shared_from_this, so it has to be shared_ptr
    channel_->EnableRead();
    channel_->UseET();
  }

  #ifdef debug
  std::cout<<"the fd of the socket is: "<<sock_->GetFd()<<std::endl;
  #endif
  read_buffer_  = std::make_unique<StreamBuffer>();
  send_buffer_ = std::make_unique<StreamBuffer>();
  state_ = State::Connected;
}

Connection::~Connection() {}


void Connection::Read() {
  ASSERT(state_ == State::Connected, "connection state is disconnected!");
  read_buffer_->Clear();
  if (sock_->IsNonBlocking()) {
    ReadNonBlocking();
  } else {
    ReadBlocking();
  }
}
void Connection::Write() {
  ASSERT(state_ == State::Connected, "connection state is disconnected!");
  if (sock_->IsNonBlocking()) {
    WriteNonBlocking();
  } else {
    WriteBlocking();
  }
  send_buffer_->Clear();
}


void Connection::ReadNonBlocking() {
  int sockfd = sock_->GetFd();
  vector<char> buf(1024);
  while (true) {   // 使用非阻塞IO，读取客户端buffer，一次读取buf大小数据，直到全部读取完毕
    // memset(buf, 0, sizeof(buf));
    buf.resize(1024);
    ssize_t bytes_read = read(sockfd, (void*)&buf[0], buf.size());
    if (bytes_read > 0) {
      // read_buffer_->Append(buf, bytes_read);
      read_buffer_->input(buf.data(), bytes_read);
    } else if (bytes_read == -1 && errno == EINTR) {  // 程序正常中断、继续读取
      printf("continue reading\n");
      continue;
    } else if (bytes_read == -1 &&
               ((errno == EAGAIN) || (errno == EWOULDBLOCK))) {  // 非阻塞IO，这个条件表示数据全部读取完毕
      break;
    } else if (bytes_read == 0) {  // EOF，客户端断开连接
      printf("read EOF, client fd %d disconnected\n", sockfd);
      state_ = State::Closed;
      break;
    } else {
      printf("Other error on client fd %d\n", sockfd);
      state_ = State::Closed;
      break;
    }
  }
}



void Connection::WriteNonBlocking() {
  int sockfd = sock_->GetFd();
  int data_size = send_buffer_->size();
  int data_left = data_size;
  while (data_left > 0) {
    ssize_t bytes_write = write(sockfd, &(*send_buffer_)[0] + data_size - data_left, data_left);
    if (bytes_write == -1 && errno == EINTR) {
      printf("continue writing\n");
      continue;
    }
    if (bytes_write == -1 && errno == EAGAIN) {
      break;
    }
    if (bytes_write == -1) {
      printf("Other error on client fd %d\n", sockfd);
      state_ = State::Closed;
      break;
    }
    data_left -= bytes_write;
  }
  
  #ifdef debug
  //server send data
  std::cout<<"the server send data size :"<<send_buffer_->size()-data_left<<endl;
  #endif
}


/**
 * @brief Never used by server, only for client
 *
 */
void Connection::ReadBlocking() {
  int sockfd = sock_->GetFd();
  unsigned int rcv_size = 1024;
  socklen_t len = sizeof(rcv_size);
  getsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &rcv_size, &len);
  vector<char> buf(rcv_size);
  ssize_t bytes_read = read(sockfd, (void*)&buf[0], buf.size());
  if (bytes_read > 0) {
    // read_buffer_->Append(buf, bytes_read);
    buf.resize(bytes_read);
    read_buffer_->input(buf.data(),bytes_read);
  } else if (bytes_read == 0) {
    printf("read EOF, blocking client fd %d disconnected\n", sockfd);
    state_ = State::Closed;
  } else if (bytes_read == -1) {
    printf("Other error on blocking client fd %d\n", sockfd);
    state_ = State::Closed;
  }
}



/**
 * @brief Never used by server, only for client
 *
 */
void Connection::WriteBlocking() {
  int sockfd = sock_->GetFd();
  ssize_t bytes_write = write(sockfd, send_buffer_->data(), send_buffer_->size());
  if (bytes_write == -1) {
    printf("Other error on blocking client fd %d\n", sockfd);
    state_ = State::Closed;
  }
  
  #ifdef debug
  std::cout<<"the send data size : "<<bytes_write<<endl;
  #endif
}

void Connection::Close() { delete_connectioin_callback_(sock_); }

Connection::State Connection::GetState() { return state_; }


void Connection::SetSendBuffer(const char*p)
{
   send_buffer_->Clear();
   send_buffer_->input(p, strlen(p));
}


void Connection::SetSendBuffer(std::string &s)
{
   send_buffer_->Clear();
   send_buffer_->input(s.data(), s.size());
}

void Connection::SetSendBuffer(std::string &&s)
{
   send_buffer_->Clear();
   send_buffer_->input(s.data(), s.size());
}



void Connection::SetSendBuffer(StreamBuffer &buf) 
{
    send_buffer_->Clear();
    send_buffer_->input(buf.data(),buf.size());
}
void Connection::SetSendBuffer(unique_ptr<StreamBuffer> buf) 
{
    send_buffer_ = std::move(buf);
}


StreamBuffer Connection::GetReadBuffer()
{
   return *read_buffer_; 
}


StreamBuffer Connection::GetSendBuffer() 
{ 
  return *send_buffer_; 
}

void Connection::SetDeleteConnectionCallback(std::function<void(const std::unique_ptr<Socket> &)> const &callback) {
  delete_connectioin_callback_ = callback;
}

void Connection::SetOnConnectCallback(std::function<void(std::shared_ptr<Connection>)> const &callback) {
  on_connect_callback_ = callback;
  //here we let the channel know what it will do, when it receive the readable signal
  channel_->SetReadCallback([this]() { on_connect_callback_(shared_from_this()); });
}

void Connection::GetlineSendBuffer() {
    string buf;
   std::getline(std::cin,buf);
   send_buffer_->Clear();
   send_buffer_->input(buf.data(), buf.size()); 
}

const std::unique_ptr<Socket> &Connection::GetSocket() { return sock_; }
