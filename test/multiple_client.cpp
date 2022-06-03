#include <unistd.h>
#include <cstring>

#include <functional>
#include <iostream>

#include <Connection.h>
#include <Socket.h>
#include <ThreadPool.h>
#include <EventLoop.h>

#include <sys/time.h>    
#include <unistd.h>   
#include <stdio.h>

void OneClient(int msgs, int wait) {
  
  unique_ptr<Socket> sock = make_unique<Socket>();
  sock->Connect("127.0.0.1", 1234);
  unique_ptr<EventLoop> loop(nullptr);
  unique_ptr<Connection> conn  =std::make_unique<Connection>(loop, std::move(sock));

  sleep(wait);
  int count = 0;
  while (count < msgs) {
    conn->SetSendBuffer("I'm client!");
    conn->Write();
    if (conn->GetState() == Connection::State::Closed) {
      conn->Close();
      break;
    }
    conn->Read();
    auto buf = conn->GetReadBuffer();
    string s(buf.begin(),buf.end());
    std::cout << "msg count " << count++ << ": " << s << std::endl;
  }
}

int main(int argc, char *argv[]) {
  int threads = 100;
  int msgs = 100;
  int wait = 0;
  int o = -1;
  const char *optstring = "t:m:w:";
  while ((o = getopt(argc, argv, optstring)) != -1) {
    switch (o) {
      case 't':
        threads = std::stoi(optarg);
        break;
      case 'm':
        msgs = std::stoi(optarg);
        break;
      case 'w':
        wait = std::stoi(optarg);
        break;
      case '?':
        printf("error optopt: %c\n", optopt);
        printf("error opterr: %d\n", opterr);
        break;
      default:
        break;
    }
  }

  ThreadPool *poll = new ThreadPool(threads);
  std::function<void()> func = std::bind(OneClient, msgs, wait);
  for (int i = 0; i < threads; ++i) {
    poll->Add(func);
  }
  delete poll;
  return 0;
}
