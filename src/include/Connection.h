#pragma once
#include "Macros.h"

#include <functional>
#include <Serializer.hpp>
#include <memory>
class EventLoop;
class Socket;
class Channel;
class Buffer;
class Connection : public std::enable_shared_from_this<Connection>
{
 public:
  enum State {
    Invalid = 1,
    Handshaking,
    Connected,
    Closed,
    Failed,
  };
  Connection(const std::unique_ptr<EventLoop> &loop, std::unique_ptr<Socket> sock);
  ~Connection();
  DISALLOW_COPY_AND_MOVE(Connection);

  void Read();
  void Write();

  void SetDeleteConnectionCallback(std::function<void(const std::unique_ptr<Socket> &)> const &callback);
  void SetOnConnectCallback(std::function<void(std::shared_ptr<Connection> )> const &callback);
  State GetState();
  void Close();
  

  void SetSendBuffer(StreamBuffer &buf); //about to abort
  void SetSendBuffer(unique_ptr<StreamBuffer> buf);
  void SetSendBuffer(const char*p);
  void SetSendBuffer(std::string &s);
  void SetSendBuffer(std::string &&s);

  

  StreamBuffer GetReadBuffer();
  StreamBuffer GetSendBuffer();
  void GetlineSendBuffer();
  const std::unique_ptr<Socket> &GetSocket();

  void OnConnect(std::function<void()> fn);

 private:
  const std::unique_ptr<EventLoop> &loop_;
  std::unique_ptr<Socket> sock_;
  std::shared_ptr<Channel> channel_;
  State state_{State::Invalid};
  std::unique_ptr<StreamBuffer> read_buffer_;
  std::unique_ptr<StreamBuffer> send_buffer_;
  std::function<void(const std::unique_ptr<Socket> &)> delete_connectioin_callback_;

  std::function<void(std::shared_ptr<Connection> )> on_connect_callback_;

  void ReadNonBlocking();
  void WriteNonBlocking();
  void ReadBlocking();
  void WriteBlocking();
};
