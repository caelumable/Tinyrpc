# Trpc - Tiny RPC framework for Linux 

+ 实现了一个小型的muduo库作为网络层
+ 使用智能指针进行了重构
+ 实现了一个简易的序列化和反序列化的编码器
+ 使用c++14特性使得RPC支持可变参数的调用





## Feature

+ 轻量
+ 自由绑定带任意参数的函数



## How to use:

```
cd Tinyrpc
mkdir build
cd build
cmake ..
cmake --build .
```

然后运行`/build/bin`下的RpcServer和RpcClient即可





## ToDo

+ 增加网络层对协程的支持
