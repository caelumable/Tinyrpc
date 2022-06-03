#include <Server.h>
#include <chrono>
#include <iostream>
#include <memory>
#include <ratio>
#include <thread>
#include <Connection.h>
#include <EventLoop.h>
#include <Serializer.hpp>
#include <Socket.h>

int main() {
  auto loop = std::make_unique<EventLoop>();
  auto server = std::make_unique<Server>(loop,1234);
  server->OnConnect([](std::shared_ptr<Connection> conn) {
    conn->Read();
    if (conn->GetState() == Connection::State::Closed) {
      conn->Close();
      return;
    }
    // std::cout << "Message from client " << conn->GetSocket()->GetFd() << ",  the data size recieved from client:  " << conn->GetReadBuffer().size() << std::endl;
    auto buf = conn->GetReadBuffer();
    string s(buf.begin(),buf.end());
    std::cout << "Message from client " << s << std::endl;
    conn->SetSendBuffer(std::move(s));
    conn->Write();
  });

  loop->Loop();
  return 0;
}
