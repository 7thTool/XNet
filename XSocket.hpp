#ifndef __H_XNET_tcp_session_HPP__
#define __H_XNET_tcp_session_HPP__

#pragma once

#include "XType.hpp"
#include "XPeer.hpp"

namespace XNet {

#if XSERVER_PROTOTYPE_TCP

// tcp_session
template <class Derived>
class tcp_session
{
	typedef tcp_session<Derived> This;
  public:
	tcp_session()
		: recv_buffer_(derived().server().max_buffer_size())
		//, send_buffer_(derived().server().max_buffer_size()), write_buffer_(derived().server().max_buffer_size())
		, write_complete(true)
	{
		recv_buffer_.ensureWritable(derived().server().max_buffer_size());
	}

	~tcp_session()
	{
	}

	inline Derived & derived() { return static_cast<Derived &>(*this); }

	inline bool is_open()
	{
		return derived().sock().is_open();
	}

	inline void close()
	{
		derived().server().post_io_callback(id(),
					   boost::bind(&Derived::do_close,
								   derived().shared_from_this()));
	}

	void do_close()
	{
		if (is_open())
		{
			boost::system::error_code ec;
			//derived().sock().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
			derived().sock().close(ec);
		}
	}

	inline void do_write(const char *buf, size_t len)
	{
		BOOST_ASSERT(is_open());
#ifdef _DEBUG
		LOG4I("XPEER(%d) WRITE %d", derived().id(), len);
#endif //
		boost::mutex::scoped_lock lock(write_mutex_);
		send_buffer_.append(buf, len);
		if (write_complete)
		{
			write_buffer_.clear();
			write_buffer_.swap(send_buffer_);
			do_write();
		}
	}

	inline void do_read()
	{
		derived().sock().async_read_some(boost::asio::buffer(recv_buffer_.writer(), recv_buffer_.writable()),
										 boost::bind(&Derived::on_read, derived().shared_from_this(),
													 boost::asio::placeholders::error,
													 boost::asio::placeholders::bytes_transferred));
	}

	void on_read(const boost::system::error_code &ec, size_t bytes_transferred)
	{
		if (!ec)
		{
#ifdef _DEBUG
			LOG4I("XPEER(%d) ON_READ %d", derived().id(), bytes_transferred);
#endif //
			/*boost::mutex::scoped_lock lock(mutex_);
			recv_buffer_.append(read_buffer_, bytes_transferred);
			lock.unlock();
			derived().server().on_io_read(shared_from_this(), read_buffer_, bytes_transferred);*/
			const char *buf = recv_buffer_.data();
			recv_buffer_.write(bytes_transferred);
			derived().server().on_io_read(derived().shared_from_this(), recv_buffer_);
			//AsyncRead(); 这里不主动读，等work处理完读到的数据再继续AsyncRead
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = derived().get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d READ ERROR: %d", derived().id(), str.c_str(), ep.port(), ec.value());
		}
	}

	inline void do_write()
	{
		write_complete = false;

		boost::asio::async_write(derived().sock(),
								 boost::asio::buffer(write_buffer_.data(), write_buffer_.size()),
								 boost::bind(&Derived::on_write, derived().shared_from_this(),
											 boost::asio::placeholders::error,
											 boost::asio::placeholders::bytes_transferred));
	}

	void on_write(const boost::system::error_code &ec, size_t bytes_transferred)
	{
		if (!ec)
		{
			BOOST_ASSERT(write_buffer_.size() == bytes_transferred);
#ifdef _DEBUG
			LOG4I("XPEER(%d) ON_WRITE %d", derived().id(), bytes_transferred);
#endif //
			derived().server().on_io_write(derived().shared_from_this(),write_buffer_);
			boost::mutex::scoped_lock lock(write_mutex_);
			write_complete = true;
			write_buffer_.clear();
			write_buffer_.swap(send_buffer_);
			if (write_buffer_.size())
				do_write();
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = derived().get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d WRITE ERROR: %d", derived().id(), str.c_str(), ep.port(), ec.value());
		}
	}

  protected:
	XBuffer recv_buffer_;
	XBuffer send_buffer_;
	XBuffer write_buffer_;
	bool write_complete;
	boost::mutex write_mutex_;
};

///

template <class Server>
class tcp_peer_session
	: public XPeer<Server, tcp_peer_session<Server>>
	, public tcp_session<tcp_peer_session<Server>>
	, public std::enable_shared_from_this<tcp_peer_session<Server>>
	, private boost::noncopyable
{
	typedef tcp_peer_session<Server> This;
	typedef XPeer<Server, tcp_peer_session<Server>> Base;
	typedef tcp_session<tcp_peer_session<Server>> Handler;
  public:
	tcp_peer_session(Server &srv, size_t id, boost::asio::ip::tcp::socket sock)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_TCP, id)), Handler(), sock_(std::move(sock))
	{
	}

	~tcp_peer_session()
	{
		server().on_io_close(this);
	}

	inline boost::asio::ip::tcp::socket &sock() { return sock_; }

	inline void run()
	{
		boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
		std::string str = ep.address().to_string();
		LOG4I("XPEER(%d) %s:%d  CONNECTED", id(), str.c_str(), ep.port());
		do_read();
	}

  protected:
	boost::asio::ip::tcp::socket sock_;
};

template <class Server>
class tcp_client_session
	: public XClientPeer<Server,tcp_client_session<Server>>
	, public tcp_session<tcp_peer_session<Server>>
	, public std::enable_shared_from_this<tcp_client_session<Server>>
	, private boost::noncopyable
{
	typedef tcp_client_session<Server> This;
	typedef XClientPeer<Server,tcp_client_session<Server>> Base;
	typedef tcp_session<tcp_peer_session<Server>> Handler;
  public:
	tcp_client_session(Server &srv, size_t id, boost::asio::ip::tcp::socket& sock)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_TCP_CLIENT, id)), Handler(), sock_(std::move(sock))
	{
	}

	~tcp_client_session()
	{
		server().on_io_close(this);
	}

	inline boost::asio::ip::tcp::socket &sock() { return sock_; }

  protected:
	boost::asio::ip::tcp::socket sock_;
};

#endif //

}

#endif //__H_XNET_tcp_session_HPP__
