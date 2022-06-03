#include <EventLoop.h>
#include <Socket.h>
#include <Connection.h>
#include <Serializer.hpp>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <Serializer.hpp>

int main() {
  unique_ptr<Socket> sock = make_unique<Socket>();
  sock->Connect("127.0.0.1", 1234);
  unique_ptr<EventLoop> loop(nullptr);
  unique_ptr<Connection> conn  =std::make_unique<Connection>(loop, std::move(sock));

  while (true) {
    conn->GetlineSendBuffer();
    conn->Write();
    if (conn->GetState() == Connection::State::Closed) {
      conn->Close();
      break;
    }
    conn->Read();
    auto buf = conn->GetReadBuffer();
    string s(buf.begin(),buf.end());
    std::cout << "Message from server: " << s << std::endl;
    // StreamBuffer buf;
    // const char *s = "foo_3";
    // buf.input(s, strlen(s));
    // cout << buf.size() << endl;
    // int x = 10;
    // int len = sizeof(int);
    // char *d = new char[len];
    // const char *p = reinterpret_cast<const char *>(&x);
    // memcpy(d, p, len);
    // buf.input(d, len);
    // delete[] d;
    // std::cout<<"the buf size: "<<buf.size()<<endl;
    // conn->SetSendBuffer(buf);
    // std::cout<<"the data size about to send: "<< conn->GetSendBuffer().size() <<endl;
    // conn->Write();
    // if (conn->GetState() == Connection::State::Closed) {
    //   conn->Close();
    //   break;
    // }
    // conn->Read();
    // std::cout << "Message size from server: " << conn->GetReadBuffer().size() << std::endl;
    // usleep(10000);
  }

  return 0;
}
