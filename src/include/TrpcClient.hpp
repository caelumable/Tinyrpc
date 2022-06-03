#pragma once
#include <string>
#include <map>
#include <string>
#include <sstream>
#include <functional>
#include <memory>
#include "Serializer.hpp"
#include <Connection.h>
#include <Socket.h>
#include <tuple>
#include <EventLoop.h>
#include <iostream>


// #define debug

class Serializer;

template<typename T>
struct type_xx{	typedef T type; };

template<>
struct type_xx<void>{ typedef int8_t type; };


template<typename Tuple, std::size_t... Is>
void package_params_impl(Serializer& ds, const Tuple& t, std::index_sequence<Is...>)
{
	initializer_list<int>{((ds << std::get<Is>(t)), 0)...};
}

template<typename... Args>
void package_params(Serializer& ds, const std::tuple<Args...>& t)
{
	package_params_impl(ds, t, std::index_sequence_for<Args...>{});
}



class TRpcClient
{
public:
	enum rpc_err_code {
		RPC_ERR_SUCCESS = 0,
		RPC_ERR_FUNCTIION_NOT_BIND,
		RPC_ERR_RECV_TIMEOUT
	};

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

	TRpcClient( std::string ip, uint16_t port);
	~TRpcClient();


public:
	template<typename R, typename... Params>
	value_t<R> call(std::string name, Params... ps) {
		using args_type = std::tuple<typename std::decay<Params>::type...>;
		args_type args = std::make_tuple(ps...);
		Serializer ds;
		ds << name;
		package_params(ds, args);
		return net_call<R>(ds);
	}

	template<typename R>
	value_t<R> call(std::string name) {
		Serializer ds;
		ds << name;
		return net_call<R>(ds);
	}

private:
	template<typename R>
	value_t<R> net_call(Serializer& ds);

private:
    unique_ptr<Connection> conn;
	rpc_err_code m_error_code;
};

inline TRpcClient::TRpcClient( std::string ip, uint16_t port)  
{
	m_error_code = RPC_ERR_SUCCESS;
    unique_ptr<Socket> sock = make_unique<Socket>();
	sock->Connect(ip.data(), port);
  	unique_ptr<EventLoop> loop(nullptr);
  	conn  =std::make_unique<Connection>(loop, std::move(sock));
}

inline TRpcClient::~TRpcClient(){
	if(conn) conn->Close();
}


template<typename R>
inline TRpcClient::value_t<R> TRpcClient::net_call(Serializer& ds)
{
    if(m_error_code != RPC_ERR_RECV_TIMEOUT)
	{
		#ifdef debug
		::std::cout<<"I'm sending the data of size: "<<ds.size()<<endl;
		#endif
		// char *SendData = new char[ds.size()];
		// memcpy(SendData,ds.data(),ds.size());
		// StreamBuffer buff(ds.data(),ds.size());
        unique_ptr<StreamBuffer> buff = make_unique<StreamBuffer>(ds.data(),ds.size());
		// string SendData(buff.begin(),buff.end());
		#ifdef debug
		::std::cout<<"the size of SendData:  "<<buff->size()<<endl;
		#endif
		conn->SetSendBuffer(std::move(buff));

		#ifdef debug
		::std::cout<<"The size about to send: "<<conn->GetSendBuffer().size()<<endl;
		#endif 

		conn->Write();
	}
	

	value_t<R> val;
	if (conn->GetState() == Connection::State::Closed) {
      conn->Close();
      return val;
    }
	conn->Read();


	StreamBuffer buf = conn->GetReadBuffer();
	#ifdef debug
	::std::cout<<"The result size received from server: "<<buf.size()<<endl;
	#endif

	if(buf.size()==0)
	{
		m_error_code = RPC_ERR_RECV_TIMEOUT;
		val.set_code(RPC_ERR_RECV_TIMEOUT);
		val.set_msg("recv timeout");
		return val;
	}
	m_error_code = RPC_ERR_SUCCESS;
	ds.clear();
	ds.write_raw_data( buf.data() , buf.size());
	ds.reset();
	ds >> val;
	return val;	
}
