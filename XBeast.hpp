#ifndef __H_XNET_XBEAST_HPP__
#define __H_XNET_XBEAST_HPP__

#pragma once

#include "XType.hpp"
#include "XUtil.hpp"
#include "XPeer.hpp"

#if XSERVER_PROTOTYPE_BEAST

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/make_unique.hpp>
#include <boost/config.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#if XSERVER_PROTOTYPE_SSL
#include <boost/asio/ssl/stream.hpp>
#include "detect_ssl.hpp"
#include "ssl_stream.hpp"
#endif//

#endif //

namespace XNet {

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET

//------------------------------------------------------------------------------

// Echoes back all received WebSocket messages.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template <class Derived>
class websocket_session : public XPeer<Derived>
{
	typedef XPeer<Derived> Base;
	// Access the derived class, this is part of
	// the Curiously Recurring Template Pattern idiom.
	//inline Derived & derived() { return static_cast<Derived &>(*this); }

	boost::beast::multi_buffer read_buffers_; //当前收到的包
	//std::string read_buffer_; //buffer_ => string
	//x_packet_t packet_; //当前处理包
	char ping_state_ = 0; //ping pong状态
	std::list<std::string> write_buffers_;
	//std::string  write_buffer_;
	bool write_complete_ = true;
	boost::mutex write_mutex_;

  protected:
	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	//boost::asio::steady_timer timer_;
	std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)>
		control_callback_;

  public:
	// Construct the session
	explicit websocket_session(size_t id, boost::asio::io_context &ioc)
		: Base(id), strand_(ioc.get_executor())
	//, timer_(ioc,(std::chrono::steady_clock::time_point::max)())
	{
	}
	~websocket_session()
	{
	}

	inline bool is_open() { return derived().ws().is_open(); }

	inline void close()
	{
		derived().service().post(boost::bind(&Derived::do_close, derived().shared_from_this()));
	}

	inline void ping()
	{
		derived().service().post(boost::bind(&Derived::do_ping, derived().shared_from_this()));
	}

	inline void do_ping()
	{
		derived().on_timer({});
	}

	inline void do_write(const char *buf, size_t len)
	{
		if (!buf || !len)
		{
			return;
		}
		boost::mutex::scoped_lock lock(write_mutex_);
		write_buffers_.emplace_back(buf, len);
		if (write_complete_)
		{
			derived().do_write();
		}
	}

	// Start the asynchronous operation
	template <class Body, class Allocator>
	inline void
	do_accept(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{
		derived().server().on_io_preaccept(derived().shared_from_this(), std::move(req));

		// Set the control callback. This will be called
		// on every incoming ping, pong, and close frame.
		/*control_callback_ = std::bind(
			&Derived::on_control_callback,
			this,
			std::placeholders::_1,
			std::placeholders::_2);
		derived().ws().control_callback(control_callback_);*/

		// Set the timer
		//timer_.expires_after(std::chrono::seconds(15));

		// Accept the websocket handshake
		derived().ws().async_accept(
			req,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_accept,
					derived().shared_from_this(),
					std::placeholders::_1)));
	}
	inline void
	do_accept()
	{
		// Set the control callback. This will be called
		// on every incoming ping, pong, and close frame.
		/*control_callback_ = std::bind(
			&Derived::on_control_callback,
			this,
			std::placeholders::_1,
			std::placeholders::_2);
		derived().ws().control_callback(control_callback_);*/

		// Set the timer
		//timer_.expires_after(std::chrono::seconds(15));

		// Accept the websocket handshake
		derived().ws().async_accept(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_accept,
					derived().shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_accept(const boost::system::error_code &ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return derived().on_fail(ec, "accept");

		// Read a message
		derived().do_read();
	}

	// Called when the timer expires.
	void
	on_timer(const boost::system::error_code &ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
			return derived().on_fail(ec, "timer");

		if (!derived().is_open())
			return;

		// See if the timer really expired since the deadline may have moved.
		//if (timer_.expiry() <= std::chrono::steady_clock::now())
		{
			// If this is the first time the timer expired,
			// send a ping to see if the other end is there.
			if (derived().is_open() && ping_state_ == 0)
			{
				// Note that we are sending a ping
				ping_state_ = 1;

				// Set the timer
				//timer_.expires_after(std::chrono::seconds(15));

				// Now send the ping
				derived().ws().async_ping({},
										  boost::asio::bind_executor(
											  strand_,
											  std::bind(
												  &Derived::on_ping,
												  derived().shared_from_this(),
												  std::placeholders::_1)));
			}
			else
			{
				// The timer expired while trying to handshake,
				// or we sent a ping and it never completed or
				// we never got back a control frame, so close.

				derived().do_timeout();
				return;
			}
		}

		// Wait on the timer
		/*timer_.async_wait(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_timer,
					derived().shared_from_this(),
					std::placeholders::_1)));*/
	}

	// Called to indicate activity from the remote peer
	inline void
	activity()
	{
		// Note that the connection is alive
		ping_state_ = 0;

		// Set the timer
		//timer_.expires_after(std::chrono::seconds(15));

		derived().server().on_io_activity(derived().shared_from_this());
	}

	// Called after a ping is sent.
	void
	on_ping(const boost::system::error_code &ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return derived().on_fail(ec, "ping");

		// Note that the ping was sent.
		if (ping_state_ == 1)
		{
			ping_state_ = 2;
		}
		else
		{
			// ping_state_ could have been set to 0
			// if an incoming control frame was received
			// at exactly the same time we sent a ping.
			BOOST_ASSERT(ping_state_ == 0);
		}
	}

	void
	on_control_callback(
		boost::beast::websocket::frame_type kind,
		boost::beast::string_view payload)
	{
		boost::ignore_unused(kind, payload);

		switch(kind)
		{
		case boost::beast::websocket::frame_type::ping:
		{
			//
		}
		break;
		case boost::beast::websocket::frame_type::pong:
		{
			//
		}
		break;
		case boost::beast::websocket::frame_type::close:
		{
			return;
		}
		default:
		break;
		}

		// Note that there is activity
		derived().activity();
	}

	inline void
	do_read()
	{
		// Read a message into our buffer
		derived().ws().async_read(
			read_buffers_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_read,
					derived().shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}

	void
	on_read(
		const boost::system::error_code &ec,
		size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		// This indicates that the websocket_session was closed
		if (ec == boost::beast::websocket::error::closed)
			return;

		// Note that there is activity
		derived().activity();

		if (!ec)
		{
			//填充multi_buffer
			//size_t n = boost::asio::detail::buffer_copy(buffer_.prepare(contents.size()), boost::asio::buffer(contents));
			//buffer_.commit(n);
			//读取buffer_
			/*std::ostringstream oss(read_buffer_);
			oss << boost::beast::buffers(read_buffers_.data());*/
			std::string buffer = boost::beast::buffers_to_string(read_buffers_.data());
			read_buffers_.consume(read_buffers_.size());
			derived().server().on_io_read(derived().shared_from_this(), buffer);
		}
		else
		{
			derived().on_fail(ec, "read");
		}
	}

	inline void do_write()
	{
		write_complete_ = false;
		derived().ws().text(derived().ws().got_text());
		derived().ws().async_write(
			boost::asio::buffer(write_buffers_.front()),
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_write,
					derived().shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}

	void
	on_write(
		const boost::system::error_code &ec,
		size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (!ec)
		{
			std::string &buffer = write_buffers_.front();
			derived().server().on_io_write(derived().shared_from_this(), buffer);
			boost::mutex::scoped_lock lock(write_mutex_);
			write_complete_ = true;
			write_buffers_.pop_front();
			if (!write_buffers_.empty())
				derived().do_write();
		}
		else
		{
			return derived().on_fail(ec, "write");
		}
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE == XSERVER_WEBSOCKET

// Handles a plain WebSocket connection
template <class Server>
class plain_websocket_session
	: public websocket_session<plain_websocket_session<Server>>
	, public std::enable_shared_from_this<plain_websocket_session<Server>>
{
	typedef plain_websocket_session<Server> This;
	typedef websocket_session<plain_websocket_session<Server>> Base;
	Server& srv_;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
	bool close_ = false;

  public:
	// Create the plain_websocket_session
	explicit plain_websocket_session(Server &srv, size_t id, boost::asio::ip::tcp::socket socket)
		: Base(MAKE_PEER_ID(PEER_TYPE_WEBSOCKET, id), socket.get_executor().context())
		, srv_(srv)
		, ws_(std::move(socket))
	{
	}

	~plain_websocket_session()
	{
		this->server().on_io_close(this);
	}

	inline Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return ws_.next_layer(); }
	inline boost::beast::websocket::stream<boost::asio::ip::tcp::socket> & ws() { return ws_; }

	inline bool is_open() { return !close_ && ws().is_open(); }

	// Start the asynchronous operation
	template <class Body, class Allocator>
	inline void
	run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		//on_timer({});

		// Accept the WebSocket upgrade request
		this->do_accept(std::move(req));
	}
	inline void
	run()
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		//on_timer({});

		// Accept the WebSocket request
		this->do_accept();
	}

	void
	do_close()
	{
		if (close_)
			return;
		close_ = true;

		//boost::system::error_code ec;
		//timer_.cancel(ec);

		// Close the WebSocket Connection
		ws().async_close(
			boost::beast::websocket::close_code::normal,
			boost::asio::bind_executor(
				this->strand_,
				std::bind(
					&This::on_close,
					this->shared_from_this(),
					std::placeholders::_1)));
	}

	inline void
	do_timeout()
	{
		// This is so the close can have a timeout
		if (close_)
			return;
		close_ = true;

		// Set the timer
		//timer_.expires_after(std::chrono::seconds(15));

		// Close the WebSocket Connection
		ws().async_close(
			boost::beast::websocket::close_code::normal,
			boost::asio::bind_executor(
				this->strand_,
				std::bind(
					&This::on_close,
					this->shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_close(const boost::system::error_code &ec)
	{
		// Happens when close times out
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return this->on_fail(ec, "close");

		// At this point the connection is gracefully closed
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE == XSERVER_SSL_WEBSOCKET

// Handles an SSL WebSocket connection
template <class Server>
class ssl_websocket_session
	: public websocket_session<ssl_websocket_session<Server>>
	, public std::enable_shared_from_this<ssl_websocket_session<Server>>
{
	typedef ssl_websocket_session<Server> This;
	typedef websocket_session<ssl_websocket_session<Server>> Base;
	Server& srv_;
	boost::beast::websocket::stream<ssl_stream<boost::asio::ip::tcp::socket>> ws_;
	//boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	bool eof_ = false;

  public:
	// Create the ssl_websocket_session
	explicit ssl_websocket_session(Server &srv, size_t id, ssl_stream<boost::asio::ip::tcp::socket> stream)
		: Base(MAKE_PEER_ID(PEER_TYPE_SSL_WEBSOCKET, id), stream.get_executor().context())
		, srv_(srv)
		, ws_(std::move(stream))
	{
	}
	explicit ssl_websocket_session(Server &srv, size_t id, boost::asio::ip::tcp::socket socket, boost::asio::ssl::context &ctx)
		: Base(MAKE_PEER_ID(PEER_TYPE_WEBSOCKET, id), socket.get_executor().context())
		, srv_(srv)
		, ws_(std::move(socket), ctx)
	{
	}

	~ssl_websocket_session()
	{
		this->server().on_io_close(this);
	}

	inline Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return ws_.next_layer().next_layer(); }
	inline ssl_stream<boost::asio::ip::tcp::socket> &ssl() { return ws_.next_layer(); }
	inline boost::beast::websocket::stream<ssl_stream<boost::asio::ip::tcp::socket>> &ws() { return ws_; }
	
	inline bool is_open() { return !eof_ && ws().is_open(); }

	void do_close()
	{
		// If this is true it means we timed out performing the shutdown
		if (eof_)
			return;

		//boost::system::error_code ec;
		//timer_.cancel(ec);

		this->do_eof();
	}

	// Start the asynchronous operation
	template <class Body, class Allocator>
	inline void
	run(boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		//on_timer({});

		// Accept the WebSocket upgrade request
		this->do_accept(std::move(req));
	}
	inline void
	run()
	{
		// Perform the SSL handshake
		ssl().async_handshake(
			boost::asio::ssl::stream_base::server,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&This::on_handshake,
					this->shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_handshake(const boost::system::error_code &ec)
	{
		if (ec)
			return on_fail(ec, "handshake");

		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		//on_timer({});

		// Accept the WebSocket request
		this->do_accept();
	}

	inline void
	do_eof()
	{
		eof_ = true;

		// Set the timer
		//timer_.expires_after(std::chrono::seconds(15));

		// Perform the SSL shutdown
		ssl().async_shutdown(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&This::on_shutdown,
					this->shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_shutdown(const boost::system::error_code &ec)
	{
		// Happens when the shutdown times out
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return on_fail(ec, "shutdown");

		// At this point the connection is closed gracefully
	}

	inline void
	do_timeout()
	{
		// If this is true it means we timed out performing the shutdown
		if (eof_)
			return;

		// Start the timer again
		//timer_.expires_at(
		//	(std::chrono::steady_clock::time_point::max)());
		//on_timer({});
		this->do_eof();
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS

template <class Server, class Body, class Allocator>
void upgrade_websocket_session(Server &srv, size_t id,
							   boost::asio::ip::tcp::socket &&socket,
							   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
{
	std::shared_ptr<plain_websocket_session<Server>> ws_ptr = std::make_shared<plain_websocket_session<Server>>(srv, id,
											  std::move(socket));
	srv.on_io_upgrade(ws_ptr);
	ws_ptr->run(std::move(req));
}

#endif

#if XSERVER_PROTOTYPE_HTTPS

template <class Server, class Body, class Allocator>
void upgrade_websocket_session(Server &srv, size_t id,
							   ssl_stream<boost::asio::ip::tcp::socket> &&stream,
							   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
{
	std::shared_ptr<ssl_websocket_session<Server>> wss_ptr = std::make_shared<ssl_websocket_session<Server>>(srv, id,
											std::move(stream));
	srv.on_io_upgrade(wss_ptr);
	wss_ptr->run(std::move(req));
}

#endif

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS

//------------------------------------------------------------------------------

// Handles an HTTP server connection.
// This uses the Curiously Recurring Template Pattern so that
// the same code works with both SSL streams and regular sockets.
template <class Derived>
class http_session : public XPeer<Derived>
{
	typedef XPeer<Derived> Base;
	// Access the derived class, this is part of
	// the Curiously Recurring Template Pattern idiom.
	//inline Derived & derived() { return static_cast<Derived &>(*this); }

	// This queue is used for HTTP pipelining.
	class queue
	{
		enum
		{
			// Maximum number of responses we will queue
			limit = 8
		};

		// The type-erased, saved work item
		struct work
		{
			virtual ~work() = default;
			virtual void operator()() = 0;
		};

		http_session &self_;
		std::vector<std::unique_ptr<work>> items_;

	  public:
		explicit queue(http_session &self)
			: self_(self)
		{
			static_assert(limit > 0, "queue limit must be positive");
			items_.reserve(limit);
		}

		// Returns `true` if we have reached the queue limit
		inline bool
		is_full() const
		{
			return items_.size() >= limit;
		}

		// Called when a message finishes sending
		// Returns `true` if the caller should initiate a read
		bool
		on_write()
		{
			BOOST_ASSERT(!items_.empty());
			auto const was_full = is_full();
			items_.erase(items_.begin());
			if (!items_.empty())
				(*items_.front())();
			return was_full;
		}

		// Called by the HTTP handler to send a response.
		template <bool isRequest, class Body, class Fields>
		void
		operator()(boost::beast::http::message<isRequest, Body, Fields> &&msg)
		{
			// This holds a work item
			struct work_impl : work
			{
				http_session &self_;
				boost::beast::http::message<isRequest, Body, Fields> msg_;

				work_impl(
					http_session &self,
					boost::beast::http::message<isRequest, Body, Fields> &&msg)
					: self_(self), msg_(std::move(msg))
				{
				}

				void
				operator()()
				{
					boost::beast::http::async_write(
						self_.derived().stream(),
						msg_,
						boost::asio::bind_executor(
							self_.strand_,
							std::bind(
								&Derived::on_write,
								self_.derived().shared_from_this(),
								std::placeholders::_1,
								msg_.need_eof())));
				}
			};

			// Allocate and store the work
			items_.emplace_back(new work_impl(self_, std::move(msg)));

			// If there was no previous work, start this one
			if (items_.size() == 1)
				(*items_.front())();
		}
	};

	std::string const &doc_root_;
	boost::beast::http::request<boost::beast::http::string_body> req_; //当前收到的请求包
	queue queue_;													   //发送队列

  protected:
	boost::asio::steady_timer timer_;
	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	boost::beast::flat_buffer buffer_;

  public:
	// Construct the session
	http_session(size_t id, boost::asio::io_context &ioc, std::string const &doc_root)
		: Base(id)
		, doc_root_(doc_root)
		, queue_(*this)
		, timer_(ioc,(std::chrono::steady_clock::time_point::max)())
		, strand_(ioc.get_executor()), buffer_()
	{
	}
	http_session(size_t id, boost::asio::io_context &ioc, boost::beast::flat_buffer buffer, std::string const &doc_root)
		: Base(id)
		, doc_root_(doc_root)
		, queue_(*this)
		, timer_(ioc,(std::chrono::steady_clock::time_point::max)())
		, strand_(ioc.get_executor()), buffer_(std::move(buffer))
	{
	}

	void close()
	{
		derived().service().post(boost::bind(&Derived::do_close,derived().shared_from_this()));
	}

	// Called by the HTTP handler to send a response.
	template <bool isRequest, class Body, class Fields>
	inline void do_write(boost::beast::http::message<isRequest, Body, Fields> &&msg)
	{
		queue_(std::move(msg));
		// boost::beast::http::async_write(
		// 			derived().stream(),
		// 			msg,
		// 			boost::asio::bind_executor(
		// 				strand_,
		// 				std::bind(
		// 					&Derived::on_write,
		// 					derived().shared_from_this(),
		// 					std::placeholders::_1,
		// 					msg.need_eof())));
	}

	inline void
	do_read()
	{
		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));

		// Read a request
		boost::beast::http::async_read(
			derived().stream(),
			buffer_,
			req_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_read,
					derived().shared_from_this(),
					std::placeholders::_1)));
	}

	// Called when the timer expires.
	void
	on_timer(const boost::system::error_code &ec)
	{
		if (ec && ec != boost::asio::error::operation_aborted)
			return derived().on_fail(ec, "timer");

		if (!derived().is_open())
			return;

		// Verify that the timer really expired since the deadline may have moved.
		if (timer_.expiry() <= std::chrono::steady_clock::now())
			return derived().do_timeout();

		// Wait on the timer
		timer_.async_wait(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&Derived::on_timer,
					derived().shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_read(const boost::system::error_code &ec)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		// This means they closed the connection
		if (ec == boost::beast::http::error::end_of_stream)
			return derived().do_eof();

		if (ec)
			return derived().on_fail(ec, "read");

		// See if it is a WebSocket Upgrade
		if (boost::beast::websocket::is_upgrade(req_))
		{
			// Transfer the stream to a new WebSocket session
			return upgrade_websocket_session(derived().server(), derived().id(),
											 derived().release_stream(),
											 std::move(req_));
		}

		derived().server().on_io_read(derived().shared_from_this(), doc_root_, std::move(req_));

		// // If we aren't at the queue limit, try to pipeline another request
		// if (!queue_.is_full())
		// 	derived().do_read();
	}

	void
	on_write(const boost::system::error_code &ec, bool close)
	{
		// Happens when the timer closes the socket
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return derived().on_fail(ec, "write");

		if (close)
		{
			// This means we should close the connection, usually because
			// the response indicated the "Connection: close" semantic.
			return derived().do_eof();
		}

		// Inform the queue that a write completed
		if (queue_.on_write())
		{
			// // Read another request
			// derived().do_read();
		}
	}
};

#endif


#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS

// Handles a plain HTTP connection
template <class Server>
class plain_http_session
	: public http_session<plain_http_session<Server>>
	, public std::enable_shared_from_this<plain_http_session<Server>>
{
	typedef plain_http_session<Server> This;
	typedef http_session<plain_http_session<Server>> Base;
	Server& srv_;
	boost::asio::ip::tcp::socket socket_;

  public:
	// Create the plain_http_session
	plain_http_session(Server &srv, size_t id,
					   boost::asio::ip::tcp::socket socket,
					   std::string const &doc_root)
		: Base(MAKE_PEER_ID(PEER_TYPE_HTTP, id),socket.get_executor().context(),doc_root)
		, srv_(srv)
		, socket_(std::move(socket))
	{
	}
	plain_http_session(Server &srv, size_t id,
					   boost::asio::ip::tcp::socket socket,
					   boost::beast::flat_buffer buffer,
					   std::string const &doc_root)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_HTTP, id),socket.get_executor().context(),std::move(buffer),doc_root)
		, srv_(srv)
		, socket_(std::move(socket))
	{
	}

	~plain_http_session()
	{
		this->server().on_io_close(this);
	}

	inline Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return socket_; }
	inline boost::asio::ip::tcp::socket & stream() { return socket_; }
	// Called by the base class
	inline boost::asio::ip::tcp::socket release_stream() { return std::move(socket_); }

	inline bool is_open() { return sock().is_open(); }

	// Start the asynchronous operation
	inline void
	run()
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		this->on_timer({});

		this->do_read();
	}

	void do_close()
	{
		return this->do_timeout();
	}

	inline void
	do_eof()
	{
		// Send a TCP shutdown
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);

		// At this point the connection is closed gracefully
	}

	inline void
	do_timeout()
	{
		// Closing the socket cancels all outstanding operations. They
		// will complete with boost::asio::error::operation_aborted
		boost::system::error_code ec;
		socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		socket_.close(ec);
	}
};

#endif


#if XSERVER_PROTOTYPE_HTTPS

// Handles an SSL HTTP connection
template <class Server>
class ssl_http_session
	: public XPeer<ssl_http_session<Server>>
	, public http_session<ssl_http_session<Server>>
	, public std::enable_shared_from_this<ssl_http_session<Server>>
{
	typedef ssl_http_session<Server> This;
	typedef XPeer<ssl_http_session<Server>> Base;
	typedef http_session<ssl_http_session<Server>> Handler;
	Server& srv_;
	ssl_stream<boost::asio::ip::tcp::socket> stream_;
	bool eof_ = false;

  public:
	// Create the http_session
	ssl_http_session(Server &srv, size_t id,
					 boost::asio::ip::tcp::socket socket,
					 boost::asio::ssl::context &ctx,
					 std::string const &doc_root)
		: Base(MAKE_PEER_ID(PEER_TYPE_HTTPS, id),socket.get_executor().context(), doc_root)
		, srv_(srv)
		, stream_(std::move(socket), ctx)
	{
	}
	ssl_http_session(Server &srv, size_t id,
					 boost::asio::ip::tcp::socket socket,
					 boost::asio::ssl::context &ctx,
					 boost::beast::flat_buffer buffer,
					 std::string const &doc_root)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_HTTPS, id),socket.get_executor().context(),std::move(buffer),doc_root)
		, srv_(srv)
		, stream_(std::move(socket), ctx)
	{
	}

	~ssl_http_session()
	{
		this->server().on_io_close(this);
	}

	inline Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return stream_.next_layer(); }
	inline ssl_stream<boost::asio::ip::tcp::socket> & stream() { return stream_; }
	// Called by the base class
	inline ssl_stream<boost::asio::ip::tcp::socket> release_stream() { return std::move(stream_); }

	inline bool is_open() { return !eof_ && sock().is_open(); }

	void do_close()
	{
		return this->do_timeout();
	}

	// Start the asynchronous operation
	inline void
	run()
	{
		// Run the timer. The timer is operated
		// continuously, this simplifies the code.
		this->on_timer({});

		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));

		// Perform the SSL handshake
		// Note, this is the buffered version of the handshake.
		stream_.async_handshake(
			boost::asio::ssl::stream_base::server,
			buffer_.data(),
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&This::on_handshake,
					this->shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}
	void
	on_handshake(
		const boost::system::error_code &ec,
		size_t bytes_used)
	{
		// Happens when the handshake times out
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return on_fail(ec, "handshake");

		// Consume the portion of the buffer used by the handshake
		buffer_.consume(bytes_used);

		this->do_read();
	}

	inline void
	do_eof()
	{
		eof_ = true;

		// Set the timer
		timer_.expires_after(std::chrono::seconds(15));

		// Perform the SSL shutdown
		stream_.async_shutdown(
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&This::on_shutdown,
					this->shared_from_this(),
					std::placeholders::_1)));
	}

	void
	on_shutdown(const boost::system::error_code &ec)
	{
		// Happens when the shutdown times out
		if (ec == boost::asio::error::operation_aborted)
			return;

		if (ec)
			return this->on_fail(ec, "shutdown");

		// At this point the connection is closed gracefully
	}

	inline void
	do_timeout()
	{
		// If this is true it means we timed out performing the shutdown
		if (eof_)
			return;

		// Start the timer again
		timer_.expires_at(
			(std::chrono::steady_clock::time_point::max)());
		this->on_timer({});
		this->do_eof();
	}
};

#endif//

#if XSERVER_PROTOTYPE_HTTPS

//------------------------------------------------------------------------------

// Detects SSL handshakes
template <class Server>
class detect_session
	: public XPeer<detect_session<Server>>
	, public std::enable_shared_from_this<detect_session<Server>>
{
	typedef detect_session<Server> This;
	typedef XPeer<detect_session<Server>> Base;
	Server& srv_;
	boost::asio::ip::tcp::socket socket_;
	boost::asio::ssl::context &ctx_;
	boost::asio::strand<boost::asio::io_context::executor_type> strand_;
	std::string const &doc_root_;
	boost::beast::flat_buffer buffer_;

  public:
	explicit
		// Detects SSL handshakes
		detect_session(Server &srv, size_t id,
					   boost::asio::ip::tcp::socket socket,		   
					   boost::asio::ssl::context &ctx,
					   std::string const &doc_root)
		: Base(PEER_ID(id)), srv_(srv), socket_(std::move(socket)), ctx_(ctx), strand_(socket_.get_executor()), doc_root_(doc_root)
	{
	}

	inline Server& server() { return srv_; }

	inline boost::asio::ip::tcp::socket &sock() { return socket_; }

	// Launch the detector
	inline void
	run()
	{
		async_detect_ssl(
			socket_,
			buffer_,
			boost::asio::bind_executor(
				strand_,
				std::bind(
					&This::on_detect,
					this->shared_from_this(),
					std::placeholders::_1,
					std::placeholders::_2)));
	}

	void
	on_detect(const boost::system::error_code &ec, boost::tribool result)
	{
		if (ec)
			return on_fail(ec, "detect");

		if (result)
		{
			// Launch SSL session
			std::make_shared<ssl_http_session<Server>>(server(), id(),
											   std::move(socket_),
											   ctx_,
											   std::move(buffer_),
											   doc_root_)
				->run();
			return;
		}

		// Launch plain session
		std::make_shared<plain_http_session<Server>>(server(), id(),
											 std::move(socket_),
											 std::move(buffer_),
											 doc_root_)
			->run();
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS

template<class Derived>
class http_client_session : public XClientPeer<Derived>
{
	typedef XClientPeer<Derived> Base;
protected:
    boost::beast::flat_buffer buffer_; // (Must persist between reads)
    boost::beast::http::request<boost::beast::http::empty_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;

public:
    // Resolver and socket require an io_context
    explicit
    http_client_session(size_t id, boost::asio::io_context &io_context, size_t timeout = 0):Base(id,io_context,timeout)
    {
    }

	inline bool is_open() { return derived().sock().is_open(); }

	void close()
	{
		derived().service().post(boost::bind(&Derived::do_close,derived().shared_from_this()));
	}

	// Called by the HTTP handler to send a response.
	template <bool isRequest, class Body, class Fields>
	inline void do_write(boost::beast::http::message<isRequest, Body, Fields> &&msg)
	{
		//queue_(std::move(msg));
	}

    // Start the asynchronous operation
    void
    run(const std::string& addr, const std::string& port)
    {
        // // Set up an HTTP GET request message
        // req_.version(version);
        // req_.method(verb);
        // req_.target(target);
        // req_.set(boost::beast::http::field::host, host);
        // req_.set(boost::beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);

        derived().do_resolve(addr, port);
    }

    void
    on_connect(boost::system::error_code ec)
    {
        if(ec) {
			derived().on_fail(ec, "connect");
            return;
		}

		derived().server().on_io_connect(derived().shared_from_this());
		
        // Send the HTTP request to the remote host
        boost::beast::http::async_write(derived().sock(), req_,
            std::bind(
                &Derived::on_write,
                derived().shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec) {
			derived().on_fail(ec, "write");
            return;
		}
        
        // Receive the HTTP response
        boost::beast::http::async_read(derived().sock(), buffer_, res_,
            std::bind(
                &Derived::on_read,
                derived().shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);
        
        if(ec) {
			derived().on_fail(ec, "read");
            return;
		}

        // Write the message to standard out
        std::cout << res_ << std::endl;
    }

	void do_close()
	{
		boost::system::error_code ec;
        // Gracefully close the socket
        derived().sock().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        // not_connected happens sometimes so don't bother reporting it.
        if(ec && ec != boost::system::errc::not_connected) {
            derived().on_fail(ec, "shutdown");
			return;
		}

        // If we get here then the connection is closed gracefully
	}
};

#endif//

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS

template<class Server>
class plain_http_client_session 
	: public http_client_session<plain_http_client_session<Server>>
	, public std::enable_shared_from_this<plain_http_client_session<Server>>
{
	typedef plain_http_client_session<Server> This;
	typedef http_client_session<plain_http_client_session<Server>> Base;
	Server& srv_;
	boost::asio::ip::tcp::socket sock_;

  public:
	// Create the plain_http_client_session
	explicit
		// Handles a plain http connection
		plain_http_client_session(Server &srv, const size_t id, boost::asio::ip::tcp::socket& sock)
		: Base(MAKE_PEER_ID(PEER_TYPE_HTTP_CLIENT, id), sock.get_executor().context())
		, srv_(srv)
		, sock_(std::move(sock))
	{
	}

	~plain_http_client_session()
	{
		this->server().on_io_close(this);
	}

	Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return sock_; }
};

#endif//

#if XSERVER_PROTOTYPE_HTTPS

template<class Derived>
class ssl_http_client_session : public http_client_session<Derived>
{
	typedef ssl_http_client_session<Derived> This;
	typedef http_client_session<Derived> Base;
    boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_;
public:
    // Resolver and stream require an io_context
    explicit
    ssl_http_client_session(boost::asio::io_context& ioc, ssl::context& ctx)
        : Base(ioc)
        , stream_(ioc, ctx)
    {
    }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return stream_.next_layer(); }
	inline boost::asio::ssl::stream<boost::asio::ip::tcp::socket> &stream() { return stream_; }

    // Start the asynchronous operation
    void
    run(const std::string& addr, const std::string& port)
    {
        // Set SNI Hostname (many hosts need this to handshake successfully)
        if(! SSL_set_tlsext_host_name(stream_.native_handle(), addr))
        {
            boost::system::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
            std::cerr << ec.message() << "\n";
            return;
        }

        Base::run(addr, port);
    }

    void
    on_connect(boost::system::error_code ec)
    {
        if(ec)
            return on_fail(ec, "connect");

        // Perform the SSL handshake
        stream_.async_handshake(
            boost::asio::ssl::stream_base::client,
            std::bind(
                &This::on_handshake,
                this->shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_handshake(boost::system::error_code ec)
    {
        if(ec)
            return on_fail(ec, "handshake");

        // Send the HTTP request to the remote host
        boost::beast::http::async_write(stream_, req_,
            std::bind(
                &This::on_write,
                this->shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return on_fail(ec, "write");
        
        // Receive the HTTP response
        boost::beast::http::async_read(stream_, buffer_, res_,
            std::bind(
                &This::on_read,
                this->shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2));
    }

    void
    on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred)
    {
        boost::ignore_unused(bytes_transferred);

        if(ec)
            return on_fail(ec, "read");

        // Write the message to standard out
        std::cout << res_ << std::endl;

        // Gracefully close the stream
        stream_.async_shutdown(
            std::bind(
                &This::on_shutdown,
                this->shared_from_this(),
                std::placeholders::_1));
    }

    void
    on_shutdown(boost::system::error_code ec)
    {
        if(ec == boost::asio::error::eof)
        {
            // Rationale:
            // http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
            ec.assign(0, ec.category());
        }
        if(ec)
            return on_fail(ec, "shutdown");

        // If we get here then the connection is closed gracefully
    }
};

#endif//

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET

// WebSocket client
template <class Derived>
class websocket_client_session : public XClientPeer<Derived>
{
	typedef XClientPeer<Derived> Base;
	boost::beast::multi_buffer read_buffers_; //当前收到的包
	//std::string read_buffer_; //buffer_ => string
	//x_packet_t packet_;
	std::list<std::string> write_buffers_;
	//std::string  write_buffer_;
	bool write_complete_ = true;
	boost::mutex write_mutex_;

  protected:
	std::function<void(boost::beast::websocket::frame_type, boost::beast::string_view)>
		control_callback_;

  public:
	// Resolver and socket require an io_context
	explicit websocket_client_session(size_t id, boost::asio::io_context &io_context, size_t timeout = 0):Base(id,io_context,timeout)
	{
	}
	~websocket_client_session()
	{
	}

	inline bool is_open() { return derived().ws().is_open(); }

	inline void close()
	{
		derived().service().post(boost::bind(&Derived::do_close,derived().shared_from_this()))
	}

	inline void ping()
	{
		derived().service().post(boost::bind(&Derived::do_ping,derived().shared_from_this()));
	}

	void do_ping()
	{
		//on_timer({});
	}

	inline void do_write(const char *buf, size_t len)
	{
		if (!buf || !len)
		{
			return;
		}
		boost::mutex::scoped_lock lock(write_mutex_);
		write_buffers_.emplace_back(buf, len);
		if (write_complete_)
		{
			derived().do_write();
		}
	}

	// Start the asynchronous operation
	inline void
	run(const std::string &addr, const std::string &port)
	{
		// Save these for later
		//text_ = text;

		// Set the control callback. This will be called
		// on every incoming ping, pong, and close frame.
		control_callback_ = std::bind(
			&Derived::on_control_callback,
			derived().shared_from_this(),
			std::placeholders::_1,
			std::placeholders::_2);
		derived().ws().control_callback(control_callback_);

		derived().do_resolve(addr, port);
	}

	inline void
	do_handshake()
	{
#if 1
		// Perform the websocket handshake
		derived().ws().async_handshake(derived().addr(), "/",
			std::bind(
				&Derived::on_handshake,
				derived().shared_from_this(),
				std::placeholders::_1));
#else
		boost::system::error_code ec;
		derived().ws().handshake(derived().addr(), "/", ec);
		derived().on_handshake(ec);
#endif //
	}

	void
	on_handshake(const boost::system::error_code &ec)
	{
		if (ec) {
			derived().on_fail(ec, "handshake");
			return;
		}

		derived().server().on_io_connect(derived().shared_from_this());
		derived().do_read();
	}

	// Called to indicate activity from the remote peer
	inline void
	activity()
	{
		derived().server().on_io_activity(derived().shared_from_this());
	}

	// Called after a pong is sent.
	void
	on_pong(const boost::system::error_code &ec)
	{
		if (ec)
			return derived().on_fail(ec, "pong");

		// Note that the pong was sent.
	}

	void
	on_control_callback(
		boost::beast::websocket::frame_type kind,
		boost::beast::string_view payload)
	{
		//boost::ignore_unused(kind, payload);

		derived().activity();

		if (kind == boost::beast::websocket::frame_type::ping)
		{
			// Now send the pong
			derived().ws().async_pong({},
									  std::bind(
										  &Derived::on_pong,
										  derived().shared_from_this(),
										  std::placeholders::_1));
		}
	}

	inline void do_write()
	{
		write_complete_ = false;
		derived().ws().text(derived().ws().got_text());
		derived().ws().async_write(
			boost::asio::buffer(write_buffers_.front()),
			std::bind(
				&Derived::on_write,
				derived().shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void
	on_write(
		const boost::system::error_code &ec,
		size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (!ec)
		{
			std::string &buffer = write_buffers_.front();
			derived().server().on_io_write(derived().shared_from_this(), buffer);
			boost::mutex::scoped_lock lock(write_mutex_);
			write_complete_ = true;
			write_buffers_.pop_front();
			if (!write_buffers_.empty())
				derived().do_write();
		}
		else
		{
			return derived().on_fail(ec, "write");
		}
	}

	inline void do_read()
	{
		// Read a message into our buffer
		derived().ws().async_read(
			read_buffers_,
			std::bind(
				&Derived::on_read,
				derived().shared_from_this(),
				std::placeholders::_1,
				std::placeholders::_2));
	}

	void
	on_read(
		const boost::system::error_code &ec,
		size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		derived().activity();

		if (!ec)
		{
			//填充buffer_
			//size_t n = boost::asio::detail::buffer_copy(buffer_.prepare(contents.size()), boost::asio::buffer(contents));
			//buffer_.commit(n);
			//读取buffer_
			/*std::ostringstream oss(read_buffer_);
			oss << boost::beast::buffers(read_buffers_.data());*/
			std::string buffer = boost::beast::buffers_to_string(read_buffers_.data());
			read_buffers_.consume(read_buffers_.size());
			derived().server().on_io_read(derived().shared_from_this(), buffer);
		}
		else
		{
			derived().on_fail(ec, "read");
		}
	}

	void do_close()
	{
		// Closing the socket cancels all outstanding operations. They
		// will complete with boost::asio::error::operation_aborted
		// boost::system::error_code ec;
		// derived().sock().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
		// derived().sock().close(ec);
		// Close the WebSocket connection
		derived().ws().async_close(boost::beast::websocket::close_code::normal,
								   std::bind(
									   &Derived::on_close,
									   derived().shared_from_this(),
									   std::placeholders::_1));
	}

	void
	on_close(const boost::system::error_code &ec)
	{
		if (ec)
			return derived().on_fail(ec, "close");

		// If we get here then the connection is closed gracefully
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE == XSERVER_WEBSOCKET

// Handles a plain WebSocket connection
template <class Server>
class plain_websocket_client_session
	: public websocket_client_session<plain_websocket_client_session<Server>>
	, public std::enable_shared_from_this<plain_websocket_client_session<Server>>
{
	typedef plain_websocket_client_session<Server> This;
	typedef websocket_client_session<plain_websocket_client_session<Server>> Base;
	Server& srv_;
	boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;

  public:
	// Create the plain_websocket_client_session
	explicit
		// Handles a plain WebSocket connection
		plain_websocket_client_session(Server &srv, const size_t id, boost::asio::ip::tcp::socket& sock)
		: Base(MAKE_PEER_ID(PEER_TYPE_WEBSOCKET_CLIENT, id), sock.get_executor().context())
		, srv_(srv)
		, ws_(std::move(sock))
	{
	}

	~plain_websocket_client_session()
	{
		this->server().on_io_close(this);
	}

	Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return ws_.next_layer(); }
	inline boost::beast::websocket::stream<boost::asio::ip::tcp::socket> & ws() { return ws_; }

	void
	on_connect(const boost::system::error_code &ec)
	{
		if (ec) {
			this->on_fail(ec, "connect");
			return;
		}

		this->do_handshake();
	}
};

#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE == XSERVER_SSL_WEBSOCKET

// Handles an SSL WebSocket connection
template <class Server>
class ssl_websocket_client_session
	: public websocket_client_session<ssl_websocket_client_session<Server>>
	, public std::enable_shared_from_this<ssl_websocket_client_session<Server>>
{
	typedef ssl_websocket_client_session<Server> This;
	typedef websocket_client_session<ssl_websocket_client_session<Server>> Base;
	Server& srv_;
	boost::beast::websocket::stream<ssl_stream<boost::asio::ip::tcp::socket>> ws_;

  public:
	// Create the ssl_websocket_client_session
	explicit ssl_websocket_client_session(Server &srv, size_t id, boost::asio::ip::tcp::socket sock, boost::asio::ssl::context &ctx)
		: Base(MAKE_PEER_ID(PEER_TYPE_SSL_WEBSOCKET_CLIENT, id), sock.get_executor().context())
		, srv_(srv)
		, ws_(std::move(sock), ctx)
	{
	}

	~ssl_websocket_client_session()
	{
		this->server().on_io_close(this);
	}

	inline Server& server() { return srv_; }

	// Called by the base class
	inline boost::asio::ip::tcp::socket &sock() { return ws_.next_layer().next_layer(); }
	inline ssl_stream<boost::asio::ip::tcp::socket> &ssl() { return ws_.next_layer(); }
	inline boost::beast::websocket::stream<ssl_stream<boost::asio::ip::tcp::socket>> &ws() { return ws_; }

	inline void do_close()
	{
		Base::do_close();
	}

	inline void
	do_connect(boost::asio::ip::tcp::resolver::results_type results)
	{
		LOG4I("XPEER(%d) %s:%s  CONNECTING", derived().id(), addr().c_str(), port().c_str());
		// Make the connection on the IP address we get from a lookup
		boost::asio::async_connect(
			sock(),
			results.begin(),
			results.end(),
			std::bind(
				&This::on_connect,
				this->shared_from_this(),
				std::placeholders::_1));
	}

	void
	on_connect(boost::system::error_code ec)
	{
		if (ec) {
			this->on_fail(ec, "connect");
			return;
		}

		do_ssl_handshake();
	}

	inline void
	do_ssl_handshake()
	{
		// Perform the SSL handshake
		ssl().async_handshake(
			boost::asio::ssl::stream_base::client,
			std::bind(
				&This::on_ssl_handshake,
				this->shared_from_this(),
				std::placeholders::_1));
	}

	void
	on_ssl_handshake(boost::system::error_code ec)
	{
		if (ec) {
			this->on_fail(ec, "ssl_handshake");
			return;
		}

		this->do_handshake();
	}
};

#endif //

}

#endif //__H_XNET_XBEAST_HPP__
