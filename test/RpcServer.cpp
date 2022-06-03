#include <TrpcServer.hpp>
#include <iostream>
#define trpc_assert(exp)                                                    \
  {                                                                            \
    if (!(exp)) {                                                              \
      std::cout << "ERROR: ";                                                  \
      std::cout << "function: " << __FUNCTION__ << ", line: " << __LINE__      \
                << std::endl;                                                  \
    }                                                                          \
  }

void foo_1() {}

void foo_2(int arg1) { trpc_assert(arg1 == 10); }

int foo_3(int arg1) {
  trpc_assert(arg1 == 10);
  return arg1 * arg1;
}

int foo_4(int arg1, std::string arg2, int arg3, float arg4) {
  trpc_assert(arg1 == 10);
  trpc_assert(arg2 == "Trpc");
  trpc_assert(arg3 == 100);
  trpc_assert((arg4 > 10.0) && (arg4 < 11.0));
  return arg1 * arg3;
}

class ClassMem {
public:
  int bar(int arg1, std::string arg2, int arg3) {
    trpc_assert(arg1 == 10);
    trpc_assert(arg2 == "Trpc");
    trpc_assert(arg3 == 100);
    return arg1 * arg3;
  }
};

struct PersonInfo {
  int age;
  std::string name;
  float height;

  // must implement
  friend Serializer &operator>>(Serializer &in, PersonInfo &d) {
    in >> d.age >> d.name >> d.height;
    return in;
  }
  friend Serializer &operator<<(Serializer &out, PersonInfo d) {
    out << d.age << d.name << d.height;
    return out;
  }
};

PersonInfo foo_5(PersonInfo d, int shooes) {
  trpc_assert(d.age == 10);
  trpc_assert(d.name == "Trpc");
  trpc_assert(d.height == 170);

  PersonInfo ret;
  ret.age = d.age + 10;
  ret.name = d.name + " is good";
  ret.height = d.height + shooes;
  return ret;
}

string foo1(string s) { return s + " ok!"; }

float foo2(float x) { return x * x * 1.f; }

int main() {
  TRpcServer server(1234);
  server.bind("foo1", foo1);
  server.bind("foo2", foo2);
  server.bind("foo_1", foo_1);
  server.bind("foo_2", foo_2);
  server.bind("foo_3", std::function<int(int)>(foo_3));
  server.bind("foo_4", foo_4);
  server.bind("foo_5", foo_5);

  ClassMem s;
  server.bind("foo_6", &ClassMem::bar, &s);

  server.run();
  return 0;
}
