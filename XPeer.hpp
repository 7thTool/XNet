#ifndef __H_XNET_XPEER_HPP__
#define __H_XNET_XPEER_HPP__

#pragma once

#include "XType.hpp"
#include "XUtil.hpp"

namespace XNet {

template<class Server, class Derived>
class XPeer
{
protected:
	Server& server_;
	size_t id_;
	boost::any context_;
public:
	XPeer(Server &srv, size_t id)
		: server_(srv), id_(id)
	{
	}

	~XPeer()
	{
	}

	inline Derived & derived() { return static_cast<Derived &>(*this); }

	inline Server &server() { return server_; }

	inline size_t id() { return id_; }

	inline void set_context(boost::any context) { context_ = context; }
	inline const boost::any &context() { return context_; }
  
	boost::asio::ip::tcp::endpoint get_remote_endpoint()
	{
		boost::system::error_code ec;
		return derived().sock().remote_endpoint(ec);
	}

	std::string get_remote_ip() const
	{
		boost::system::error_code ec;
		auto endpoint = derived().sock().remote_endpoint(ec);
		if (ec)
			return "";
		auto address = endpoint.address();
		return address.to_string();
	};

	unsigned short get_remote_port() const
	{
		boost::system::error_code ec;
		auto endpoint = derived().sock().remote_endpoint(ec);
		if (ec)
			return (unsigned short)-1;
		return endpoint.port();
	};

	std::string get_local_ip() const
	{
		boost::system::error_code ec;
		auto endpoint = derived().sock().local_endpoint(ec);
		if (ec)
			return "";
		auto address = endpoint.address();
		return address.to_string();
	}

	unsigned short get_local_port() const
	{
		boost::system::error_code ec;
		auto endpoint = derived().sock().local_endpoint(ec);
		if (ec)
			return (unsigned short)-1;
		return endpoint.port();
	}

	inline void on_fail(boost::system::error_code ec, char const *what)
	{
		boost::asio::ip::tcp::endpoint ep = derived().get_remote_endpoint();
		std::string str = ep.address().to_string();
		LOG4E("peer(%d) %s:%d waht=%s error=%s", id(), str.c_str(), ep.port(), what, ec.message().c_str());
	}
};

template<class Derived>
class XResolver
{
protected:
	std::string addr_;
	std::string port_;
	boost::asio::ip::tcp::resolver resolver_;
	//boost::asio::ip::tcp::resolver::query query_;
public:
	XResolver(boost::asio::io_context& io_context)
		: resolver_(io_context)
		//, query_(boost::asio::ip::tcp::v4(), addr, port)
	{
		
	}

	inline Derived& derived() { return static_cast<Derived&>(*this); }

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
};

template<class Server, class Derived>
class XClientPeer 
	: public XPeer<Server,Derived>
	, public XResolver<Derived>
{
	typedef XPeer<Server,Derived> Base;
public:
	typedef XResolver<Derived> Resolver;
protected:
	boost::asio::steady_timer timer_;
	size_t timeout_;
	bool reconnect_;
public:
	XClientPeer(Server &srv, size_t id, boost::asio::io_context &io_context, size_t timeout = 0)
		: Base(srv, id), Resolver(io_context)
		, timer_(io_context,(std::chrono::steady_clock::time_point::max)()), timeout_(timeout), reconnect_(false)
	{
	}

	~XClientPeer()
	{
	}

	inline Derived& derived() { return static_cast<Derived&>(*this); }

	inline void set_reconnect_timeout(size_t millis) { timeout_ = millis; }
	inline size_t get_reconnect_timeout() { return timeout_; }

 protected:
 	// Start the asynchronous operation
	void run(const std::string &addr, const std::string &port)
	{
		reconnect_ = false;
		derived().do_resolve(addr, port);
	}

	void reconnect()
	{
		reconnect_ = false;
		derived().run(addr(), port());
	}

	void do_close() {
		if(reconnect_) {
			reconnect_ = false;
			boost::system::error_code ec;
			timer_.cancel(ec);
		}
	}

	void on_resolve(const boost::system::error_code& ec
		, boost::asio::ip::tcp::resolver::results_type results)
	{
		if (ec) {
			derived().do_reconnect();
			return;
		}

		derived().do_connect(results);
	}

	void do_connect(const boost::asio::ip::tcp::resolver::results_type &results)
	{
		LOG4I("XPEER(%d) %s:%s  CONNECTING", id(), addr().c_str(), port().c_str());
		//sock_.async_connect(ep, boost::bind(&XConnector::on_connect
		//	, shared_from_this()
		//	, boost::asio::placeholders::error));
		// Make the connection on the IP address we get from a lookup
		boost::asio::async_connect(
			derived().sock(),
			results.begin(),
			results.end(),
			std::bind(
				&Derived::on_connect,
				derived().shared_from_this(),
				std::placeholders::_1));
	}

	void on_connect(const boost::system::error_code &ec)
	{
		if (!ec)
		{
			server().on_io_connect(shared_from_this());
			derived().sock().set_option(boost::asio::ip::tcp::no_delay(true));
			derived().do_read();
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d CONNECT ERROR: %d", id(), str.c_str(), ep.port(), ec.value());
			derived().do_reconnect();
		}
	}

	inline void do_reconnect() {
		if(!timeout_) {
			return;
		}
		if(reconnect_) {
			return;
		}
		reconnect_ = false;

		timer_.expires_after(std::chrono::milliseconds(timeout_));
		on_reconnect_timer({});
	}

	// Called when the timer expires.
	void on_reconnect_timer(const boost::system::error_code &ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
			return derived().on_fail(ec, "reconnect_timer");

		if (!derived().is_open()) 
			return;

		// Verify that the timer really expired since the deadline may have moved.
		if (timer_.expiry() <= std::chrono::steady_clock::now())
			return derived().run();

		// Wait on the timer
		timer_.async_wait(
			//boost::asio::bind_executor(
			//	strand_,
				std::bind(
					&Derived::on_reconnect_timer,
					derived().shared_from_this(),
					std::placeholders::_1)
			//	)
			);
	}
};

}

#endif//__H_XNET_XPEER_HPP__
