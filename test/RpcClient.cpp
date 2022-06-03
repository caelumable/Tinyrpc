#include <TrpcClient.hpp>
#include <iostream>
#include <unistd.h>

#define trpc_assert(exp)                                                       \
  {                                                                            \
    if (!(exp)) {                                                              \
      std::cout << "ERROR: ";                                                  \
      std::cout << "function: " << __FUNCTION__ << ", line: " << __LINE__      \
                << std::endl;                                                  \
    }                                                                          \
  }

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

int main() {
  TRpcClient client("127.0.0.1", 1234);
  int callcnt = 0;
  while (1) {
    std::cout << "current call count: " << ++callcnt << std::endl;
    auto s = client.call<string>("foo1", "Hello world").val();
    trpc_assert(s == "Hello world ok!");

    //注意隐式类型转换,默认情况下小数是double,那么模版推导就是double,但是现在却用float来接受,那么就错了
    auto v = client.call<float>("foo2", 3.1f).val();
    trpc_assert(fabs(v - 9.61) < 0.000001);

    int foo3r = client.call<int>("foo_3", 10).val();
    trpc_assert(foo3r == 100);

    int foo4r = client.call<int>("foo_4", 10, "Trpc", 100, 10.8f).val();
    trpc_assert(foo4r == 1000);

    PersonInfo dd = {10, "Trpc", 170};
    dd = client.call<PersonInfo>("foo_5", dd, 5).val();
    trpc_assert(dd.age == 20);
    trpc_assert(dd.name == string("Trpc is good"));
    trpc_assert(fabs(dd.height - 175) < 0.000001);

    int foo6r = client.call<int>("foo_6", 10, "Trpc", 100).val();
    trpc_assert(foo6r == 1000);

    TRpcClient::value_t<void> xx = client.call<void>("foo_7", 666);
    trpc_assert(!xx.valid());
    sleep(5);
  }
  return 0;
}