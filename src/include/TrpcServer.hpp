#pragma once
#include <string>
#include <cstdint>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <memory>
#include "Serializer.hpp"
#include <EventLoop.h>
#include <Socket.h>
#include <Server.h>
#include <Connection.h>
#include <iostream>
#include <unordered_map>

// #define debug


class Serializer;

template<typename T>
struct type_xx{	typedef T type; };

template<>
struct type_xx<void>{ typedef int8_t type; };


template<typename Function, typename Tuple, std::size_t... Index>
decltype(auto) invoke_impl(Function&& func, Tuple&& t, std::index_sequence<Index...>)
{
	return func(std::get<Index>(std::forward<Tuple>(t))...);
}

template<typename Function, typename Tuple>
decltype(auto) invoke(Function&& func, Tuple&& t)
{
	constexpr auto size = std::tuple_size<typename std::decay<Tuple>::type>::value;
	return invoke_impl(std::forward<Function>(func), std::forward<Tuple>(t), std::make_index_sequence<size>{});
}

template<typename R, typename F, typename ArgsTuple>
typename std::enable_if<std::is_same<R, void>::value, typename type_xx<R>::type >::type
call_helper(F f, ArgsTuple args) {
	invoke(f, args);
	return 0;
}

template<typename R, typename F, typename ArgsTuple>
typename std::enable_if<!std::is_same<R, void>::value, typename type_xx<R>::type >::type
call_helper(F f, ArgsTuple args) {
	return invoke(f, args);
}

class TRpcServer
{
public:
	enum rpc_err_code {
		RPC_ERR_SUCCESS = 0,
		RPC_ERR_FUNCTIION_NOT_BIND,
		RPC_ERR_RECV_TIMEOUT
	};

	// wrap return value
	template<typename T>
	class value_t {
	public:
		typedef typename type_xx<T>::type type;
		typedef std::string msg_type;
		typedef uint16_t code_type;

		value_t() { code_ = 0; msg_.clear(); }
		bool valid() { return (code_ == 0 ? true : false); }
		int error_code() { return code_; }
		std::string error_msg() { return msg_; }
		type val() { return val_; }

		void set_val(const type& val) { val_ = val; }
		void set_code(code_type code) { code_ = code; }
		void set_msg(msg_type msg) { msg_ = msg; }

		friend Serializer& operator >> (Serializer& in, value_t<T>& d) {
			in >> d.code_ >> d.msg_;
			if (d.code_ == 0) {
				in >> d.val_;
			}
			return in;
		}
		friend Serializer& operator << (Serializer& out, value_t<T> d) {
			out << d.code_ << d.msg_ << d.val_;
			return out;
		}
	private:
		code_type code_;
		msg_type msg_;
		type val_;
	};

	TRpcServer(uint16_t port);
	~TRpcServer();

	void run();

public:
	template<typename F>
	void bind(std::string name, F func);

	template<typename F, typename S>
	void bind(std::string name, F func, S* s);

private:
	Serializer* call_(std::string name, const char* data, int len);


	template<typename F>
	void callproxy(F fun, Serializer* pr, const char* data, int len);

	template<typename F, typename S>
	void callproxy(F fun, S* s, Serializer* pr, const char* data, int len);

	// 函数指针
	template<typename R, typename... Params>
	void callproxy_(R(*func)(Params...), Serializer* pr, const char* data, int len) {
		callproxy_(std::function<R(Params...)>(func), pr, data, len);
	}

	// 类成员函数指针
	template<typename R, typename C, typename S, typename... Params>
	void callproxy_(R(C::* func)(Params...), S* s, Serializer* pr, const char* data, int len) {

		using args_type = std::tuple<typename std::decay<Params>::type...>;

		Serializer ds(StreamBuffer(data, len));
		constexpr auto N = std::tuple_size<typename std::decay<args_type>::type>::value;
		args_type args = ds.get_tuple < args_type >(std::make_index_sequence<N>{});

		auto ff = [=](Params... ps)->R {
			return (s->*func)(ps...);
		};
		typename type_xx<R>::type r = call_helper<R>(ff, args);

		value_t<R> val;
		val.set_code(RPC_ERR_SUCCESS);
		val.set_val(r);
		(*pr) << val;
	}

	// functional
	template<typename R, typename... Params>
	void callproxy_(std::function<R(Params... ps)> func, Serializer* pr, const char* data, int len) {
		
		using args_type = std::tuple<typename std::decay<Params>::type...>;

		Serializer ds(StreamBuffer(data, len));
		constexpr auto N = std::tuple_size<typename std::decay<args_type>::type>::value;
		args_type args = ds.get_tuple < args_type > (std::make_index_sequence<N>{});

		typename type_xx<R>::type r = call_helper<R>(func, args);

		value_t<R> val;
		val.set_code(RPC_ERR_SUCCESS);
		val.set_val(r);
		(*pr) << val;
	}

private:
	std::unordered_map<std::string, std::function<void(Serializer*, const char*, int)>> m_handlers;
    std::unique_ptr<EventLoop> loop;
    std::unique_ptr<Server> server;

	rpc_err_code m_error_code;

};

inline TRpcServer::TRpcServer(uint16_t port) 
{
	m_error_code = RPC_ERR_SUCCESS;
    loop = std::make_unique<EventLoop>();
    // server = new Server(loop,port);
  	server = std::make_unique<Server>(loop,port);
	server->OnConnect([this](std::shared_ptr<Connection> conn) {
        conn->Read();
        if (conn->GetState() == Connection::State::Closed) 
		{
 	       	conn->Close();
        	return;
        }
        // std::cout << "Message from client " << conn->GetSocket()->GetFd() << ": " << conn->ReadBuffer() << std::endl;
		auto buf = conn->GetReadBuffer();

		#ifdef debug
		// ::std::cout<<"recieved from client: "<<data->ToStr()<<endl;
		//下面只获得了数据大小为1的buffer
		//client.call<void>("foo_3",10)如果正确的话,返回的长度应该是11
		::std::cout<<"data.size():  "<<buf.size()<<endl;
		#endif

		// StreamBuffer iodev((const char*)data->ToStr(), data->Size());
		Serializer ds(buf);

		std::string funname;
		ds >> funname;
		// std::cout<<"funname: "<<funname<<endl;
		// Serializer* r = call_(funname, ds.current(), ds.size()- funname.size());
		//注意,这里的call_是外面的函数,能不能被[]捕获到都不好说
		Serializer* r = call_(funname, ds.current(), ds.size()- funname.size());
		StreamBuffer d(r->data(),r->size());
		conn->SetSendBuffer(d);

		#ifdef debug
		::std::cout<<"The calculated value in server: "<<*(int*)&d[0]<<endl;
		::std::cout<<"The calculated result size about to send: "<<d.size()<<endl;
		#endif
        // conn->SetSendBuffer(r->data());
        conn->Write();
    });
}

inline TRpcServer::~TRpcServer(){
}


inline void TRpcServer::run()
{
	// only server can call
    #ifdef debug
	for(auto it=m_handlers.begin();it!=m_handlers.end();it++)
	{
		::std::cout<<it->first<<endl;
	}
	#endif
    loop->Loop();
}


inline Serializer* TRpcServer::call_(std::string name, const char* data, int len)
{
	Serializer* ds = new Serializer();
	if (m_handlers.find(name) == m_handlers.end()) {
		(*ds) << value_t<int>::code_type(RPC_ERR_FUNCTIION_NOT_BIND);
		(*ds) << value_t<int>::msg_type("function not bind: " + name);
		return ds;
	}
	auto fun = m_handlers[name];
	fun(ds, data, len);
	ds->reset();
	return ds;
}

template<typename F>
inline void TRpcServer::bind( std::string name, F func )
{
	m_handlers[name] = std::bind(&TRpcServer::callproxy<F>, this, func, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F, typename S>
inline void TRpcServer::bind(std::string name, F func, S* s)
{
	m_handlers[name] = std::bind(&TRpcServer::callproxy<F, S>, this, func, s, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
}

template<typename F>
inline void TRpcServer::callproxy( F fun, Serializer* pr, const char* data, int len )
{
	callproxy_(fun, pr, data, len);
}

template<typename F, typename S>
inline void TRpcServer::callproxy(F fun, S * s, Serializer * pr, const char * data, int len)
{
	callproxy_(fun, s, pr, data, len);
}