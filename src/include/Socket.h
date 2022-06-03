#pragma once
#include <arpa/inet.h>
#include <memory>
#include "Macros.h"

class InetAddress {
 public:
  InetAddress();
  InetAddress(const char *ip, uint16_t port);
  ~InetAddress() = default;

  DISALLOW_COPY_AND_MOVE(InetAddress);

  void SetAddr(sockaddr_in addr);
  sockaddr_in GetAddr();
  const char *GetIp();
  uint16_t GetPort();

 private:
  struct sockaddr_in addr_ {};
};

class Socket {
 private:
  int fd_{-1};

 public:
  Socket();
  explicit Socket(int fd);
  ~Socket();

  DISALLOW_COPY_AND_MOVE(Socket);

  void Bind(const std::unique_ptr<InetAddress> &addr);
  void Listen();
  int Accept(const std::unique_ptr<InetAddress> &addr);

  void Connect(const std::unique_ptr<InetAddress> &addr);
  void Connect(const char *ip, uint16_t port);

  void SetNonBlocking();
  bool IsNonBlocking();
  int GetFd();
};
