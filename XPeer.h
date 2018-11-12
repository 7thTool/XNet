#ifndef __H_XPEER_H__
#define __H_XPEER_H__

#pragma once

#include "XType.h"

namespace XNet {

template<class Server>
class XPeer
{
public:
  XPeer(Server &srv, size_t id)
	  : server_(srv), id_(id)
  {
  }

  ~XPeer()
  {
  }

  inline Server &server() { return server_; }

  inline size_t id() { return id_; }

  inline void set_context(boost::any context)
  {
	  context_ = context;
  }

  inline const boost::any &context()
  {
	  return context_;
  }

  inline void on_fail(boost::system::error_code ec, char const *what)
  {
	  fail(ec, what);
  }

protected:
	Server& server_;
	size_t id_;
	boost::any context_;
};

template<class Server, class Derived>
class XResolver
{
public:
	XResolver(boost::asio::io_context& io_context)
		: resolver_(io_context)
		//, query_(boost::asio::ip::tcp::v4(), addr, port)
	{
		
	}

	inline Derived&
		derived()
	{
		return static_cast<Derived&>(*this);
	}

	inline std::string& addr() {
		return addr_;
	}

	inline std::string& port() {
		return port_;
	}

protected:
	void do_resolve(const std::string& addr, const std::string& port)
	{
		//resolver_.async_resolve(query_, boost::bind(&XResolver::on_resolve,
		//	derived().shared_from_this(),
		//	boost::asio::placeholders::error, _2));
		
		addr_ = addr;
		port_ = port;

		// Look up the domain name
		resolver_.async_resolve(
			addr_,
			port_,
			std::bind(
				&Derived::on_resolve,
				derived().shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void on_resolve(const boost::system::error_code& ec
		, boost::asio::ip::tcp::resolver::results_type results)
	{
		if (ec) {
			derived().on_fail(ec, "resolve");
			return;
		}

		derived().do_connect(results);
	}
protected:
	std::string addr_;
	std::string port_;
	boost::asio::ip::tcp::resolver resolver_;
	//boost::asio::ip::tcp::resolver::query query_;
};

}

#endif//__H_XPEER_H__
