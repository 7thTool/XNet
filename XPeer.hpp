#ifndef __H_XNET_XPEER_HPP__
#define __H_XNET_XPEER_HPP__

#pragma once

#include "XType.hpp"
#include "XUtil.hpp"

namespace XNet {

template<class Derived>
class XPeer
{
protected:
	size_t id_;
	boost::any context_;
public:
	XPeer(size_t id): id_(id)
	{
	}

	~XPeer()
	{
	}

	inline Derived & derived() { return static_cast<Derived &>(*this); }

	inline size_t id() { return id_; }

	inline void set_context(boost::any context) { context_ = context; }
	inline const boost::any &context() { return context_; }

	inline boost::asio::io_service& service() { return derived().sock().get_executor().context(); }
  
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

//protected:
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

template<class Derived>
class XClientPeer 
	: public XPeer<Derived>
	, public XResolver<Derived>
{
	typedef XPeer<Derived> Base;
public:
	typedef XResolver<Derived> Resolver;
protected:
	boost::asio::steady_timer connect_timer_;
	size_t connect_timeout_;
	//bool reconnect_;
public:
	XClientPeer(size_t id, boost::asio::io_context &io_context, size_t timeout = 0)
		: Base(id), Resolver(io_context)
		, connect_timer_(io_context,(std::chrono::steady_clock::time_point::max)()), connect_timeout_(timeout)
		//, reconnect_(false)
	{
	}

	~XClientPeer()
	{
	}

	inline Derived& derived() { return static_cast<Derived&>(*this); }

	inline void set_connect_timeout(size_t millis) { connect_timeout_ = millis; }
	inline size_t get_connect_timeout() { return connect_timeout_; }

 //protected:
	void on_resolve(const boost::system::error_code& ec
		, boost::asio::ip::tcp::resolver::results_type results)
	{
		if (ec) {
			return;
		}

		derived().do_connect(results);
	}

	void do_connect(const boost::asio::ip::tcp::resolver::results_type &results)
	{
		LOG4I("XPEER(%d) %s:%s  CONNECTING", derived().id(), derived().addr().c_str(), derived().port().c_str());
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
			derived().cancel_connect_timer();
			derived().server().on_io_connect(derived().shared_from_this());
			derived().sock().set_option(boost::asio::ip::tcp::no_delay(true));
			derived().do_read();
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = derived().get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d CONNECT ERROR: %d", derived().id(), str.c_str(), ep.port(), ec.value());
		}
	}

	inline void do_connect_timer() {
		if(!connect_timeout_) {
			return;
		}

		connect_timer_.expires_after(std::chrono::milliseconds(connect_timeout_));
		derived().on_connect_timeout({});
	}

	inline void cancel_connect_timer() {
		boost::system::error_code ec;
		connect_timer_.cancel(ec);
	}

	inline void do_connect_timeout() {
		derived().do_close();
		derived().run(derived().addr(), derived().port());
	}

	// Called when the timer expires.
	void on_connect_timeout(const boost::system::error_code &ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
			return derived().on_fail(ec, "on_connect_timeout");

		if (!derived().is_open()) 
			return;

		// Verify that the timer really expired since the deadline may have moved.
		if (connect_timer_.expiry() <= std::chrono::steady_clock::now()) 
			return derived().do_connect_timeout();

		// Wait on the timer
		connect_timer_.async_wait(
			//boost::asio::bind_executor(
			//	strand_,
				std::bind(
					&Derived::on_connect_timeout,
					derived().shared_from_this(),
					std::placeholders::_1)
			//	)
			);
	}
};

}

#endif//__H_XNET_XPEER_HPP__
