#ifndef __H_XNET_XSOCKET_HPP__
#define __H_XNET_XSOCKET_HPP__

#pragma once

#include "XType.hpp"
#include "XBuffer.hpp"
#include "XPeer.hpp"
#include "XLogger.hpp"

namespace XNet {

#if XSERVER_PROTOTYPE_TCP

// XSocket
template <class Server, class Derived>
class XSocket : public XPeer<Server>
{
	typedef XSocket<Server,Derived> This;
	typedef XPeer<Server> Base;
  public:
	XSocket(Server &srv, size_t id)
		: Base(srv, id), recv_buffer_(srv.max_buffer_size())
		  //, send_buffer_(srv.max_buffer_size()), write_buffer_(srv.max_buffer_size())
		  ,
		  write_complete(true)
	{
		recv_buffer_.ensureWritable(srv.max_buffer_size());
	}

	~XSocket()
	{
	}

	Derived &
	derived()
	{
		return static_cast<Derived &>(*this);
	}

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

	void on_fail(boost::system::error_code ec, char const *what)
	{
		boost::asio::ip::tcp::endpoint ep = derived().get_remote_endpoint();
		std::string str = ep.address().to_string();
		LOG4E("peer(%d) %s:%d waht=%s error=%s", id(), str.c_str(), ep.port(), what, ec.message().c_str());
	}

	bool is_open()
	{
		return derived().sock().is_open();
	}

	void close()
	{
		server().PostIO(id(),
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

	// x_packet_t& packet()
	// {
	// 	if (packet_.empty()) {
	// 		int len = server().parse_buffer(derived().shared_from_this(), recv_buffer_.data(), recv_buffer_.size());
	// 		if (len > 0) {
	// 			/*//内存整理
	// 			if (recv_buffer_.capacity()>10*server().max_buffer_size_) {
	// 				recv_buffer_.shrink();
	// 			}*/
	// 			packet_.req_data = recv_buffer_.data();
	// 			packet_.req_size = len;
	// 			packet_.create_time = boost::posix_time::microsec_clock::local_time();
	// 		}
	// 		else if (len < 0) {
	// 			boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
	// 			std::string str = ep.address().to_string();
	// 			LOG4E("XPEER(%d) %s:%d READ PACKAGE ERROR", id(), str.c_str(), ep.port());
	// 			derived().do_close();
	// 		}
	// 		else {
	// 			do_read(); //需要继续读
	// 		}
	// 	}
	// 	return packet_;
	// }

	// void response(const char* buf, size_t len, bool is_last)
	// {
	// 	if (is_last) {
	// 		packet_.destroy_time = boost::posix_time::microsec_clock::local_time();
	// 	}
	// 	if (buf && len) {
	// 		do_write(buf, len);
	// 	}
	// }

	// void on_packet_complete()
	// {
	// 	//请求处理完成
	// 	size_t req_size = packet_.req_size;
	// 	//清除请求
	// 	packet_.clear();
	// 	//移出已处理请求缓存
	// 	recv_buffer_.retrieve(req_size);
	// }

	void do_write(const char *buf, size_t len)
	{
		BOOST_ASSERT(is_open());
#ifdef _DEBUG
		LOG4I("XPEER(%d) WRITE %d", id(), len);
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

	void do_read()
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
			LOG4I("XPEER(%d) ON_READ %d", id(), bytes_transferred);
#endif //
			/*boost::mutex::scoped_lock lock(mutex_);
			recv_buffer_.append(read_buffer_, bytes_transferred);
			lock.unlock();
			server().on_io_read(shared_from_this(), read_buffer_, bytes_transferred);*/
			const char *buf = recv_buffer_.data();
			recv_buffer_.write(bytes_transferred);
			server().on_io_read(derived().shared_from_this(), recv_buffer_);
			//AsyncRead(); 这里不主动读，等work处理完读到的数据再继续AsyncRead
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d READ ERROR: %d", id(), str.c_str(), ep.port(), ec.value());
			derived().do_close();
		}
	}

	void do_write()
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
			LOG4I("XPEER(%d) ON_WRITE %d", id(), bytes_transferred);
#endif //
			server().on_io_write(derived().shared_from_this(), write_buffer_);
			boost::mutex::scoped_lock lock(write_mutex_);
			write_complete = true;
			write_buffer_.clear();
			write_buffer_.swap(send_buffer_);
			if (write_buffer_.size())
				do_write();
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d WRITE ERROR: %d", id(), str.c_str(), ep.port(), ec.value());
			derived().do_close();
		}
	}

  protected:
	//x_packet_t packet_;
	//char read_buffer_[1024];
	XRWBuffer recv_buffer_;
	XRWBuffer send_buffer_;
	XRWBuffer write_buffer_;
	bool write_complete;
	boost::mutex write_mutex_;
};

///

template <class Server>
class XWorker
	: public XSocket<Server, XWorker<Server>>,
	  public std::enable_shared_from_this<XWorker<Server>>,
	  private boost::noncopyable
{
	typedef XWorker<Server> This;
	typedef XSocket<Server, XWorker<Server>> Base;
  public:
	XWorker(Server &srv, size_t id, boost::asio::ip::tcp::socket sock)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_TCP, id)), sock_(std::move(sock))
	{
	}

	~XWorker()
	{
		server().on_io_close(this);
	}

	boost::asio::ip::tcp::socket &sock()
	{
		return sock_;
	}

	void run()
	{
		boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
		std::string str = ep.address().to_string();
		LOG4I("XPEER(%d) %s:%d  CONNECTED", id(), str.c_str(), ep.port());
		return do_read();
	}

  protected:
	boost::asio::ip::tcp::socket sock_;
};

template <class Server>
class XConnector
	: public XSocket<Server, XConnector<Server>>,
	  public XResolver<Server, XConnector<Server>>,
	  public std::enable_shared_from_this<XConnector<Server>>,
	  private boost::noncopyable
{
	typedef XConnector<Server> This;
	typedef XSocket<Server, XConnector<Server>> Base;
	typedef XResolver<Server, XConnector<Server>> Resolver;
  public:
	XConnector(Server &srv, size_t id, boost::asio::ip::tcp::socket sock)
		: Base(srv, MAKE_PEER_ID(PEER_TYPE_TCP_CLIENT, id)), Resolver(sock.get_executor().context()), sock_(std::move(sock))
	{
	}

	~XConnector()
	{
		server().on_io_close(this);
	}

	boost::asio::ip::tcp::socket &sock()
	{
		return sock_;
	}

	void run(const std::string &addr, const std::string &port)
	{
		do_resolve(addr, port);
	}

	void do_connect(const boost::asio::ip::tcp::resolver::results_type &results)
	{
		LOG4I("XPEER(%d) %s:%s  CONNECTING", id(), addr().c_str(), port().c_str());
		//sock_.async_connect(ep, boost::bind(&XConnector::on_connect
		//	, shared_from_this()
		//	, boost::asio::placeholders::error));
		// Make the connection on the IP address we get from a lookup
		boost::asio::async_connect(
			sock_,
			results.begin(),
			results.end(),
			std::bind(
				&This::on_connect,
				shared_from_this(),
				std::placeholders::_1));
	}

  protected:
	void on_connect(const boost::system::error_code &ec)
	{
		if (!ec)
		{
			server().on_io_connect(shared_from_this());
			sock_.set_option(boost::asio::ip::tcp::no_delay(true));
			do_read();
		}
		else
		{
			boost::asio::ip::tcp::endpoint ep = get_remote_endpoint();
			std::string str = ep.address().to_string();
			LOG4E("XPEER(%d) %s:%d CONNECT ERROR: %d", id(), str.c_str(), ep.port(), ec.value());
			do_close();
		}
	}

  protected:
	boost::asio::ip::tcp::socket sock_;
};

#endif //

}

#endif //__H_XNET_XSOCKET_HPP__
