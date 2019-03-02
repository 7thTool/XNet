#ifndef __H_XNET_XIDLESERVICE_H_
#define __H_XNET_XIDLESERVICE_H_

#pragma once

#include "XType.hpp"
#include "XSocket.hpp"
#include "XBeast.hpp"
#include "XUtil.hpp"
#include <boost/circular_buffer.hpp>
#include <boost/unordered_set.hpp>
#include <boost/version.hpp>

namespace XNet {

//timing wheel service
template <class Server>
class XIdleService
{
#if XSERVER_PROTOTYPE_TCP 
typedef tcp_peer_session<Server> tcp_t; 
typedef tcp_client_session<Server> tcp_clt_t; 
typedef std::shared_ptr<tcp_t> tcp_ptr; 
typedef std::weak_ptr<tcp_t> tcp_weak_ptr; 
typedef std::shared_ptr<tcp_clt_t> tcp_clt_ptr; 
typedef std::weak_ptr<tcp_clt_t> tcp_clt_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
typedef plain_http_session<Server> http_t; 
typedef std::shared_ptr<http_t> http_ptr; 
typedef std::weak_ptr<http_t> http_weak_ptr;  
#endif 
#if XSERVER_PROTOTYPE_HTTPS
typedef detect_session<Server> detect_t; 
typedef ssl_http_session<Server> https_t;  
typedef std::shared_ptr<https_t> https_ptr; 
typedef std::weak_ptr<https_t> https_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET 
typedef plain_websocket_session<Server> ws_t; 
typedef plain_websocket_client_session<Server> ws_clt_t; 
typedef std::shared_ptr<ws_t> ws_ptr; 
typedef std::weak_ptr<ws_t> ws_weak_ptr; 
typedef std::shared_ptr<ws_clt_t> ws_clt_ptr; 
typedef std::weak_ptr<ws_clt_t> ws_clt_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET 
typedef ssl_websocket_session<Server> wss_t; 
typedef ssl_websocket_client_session<Server> wss_clt_t; 
typedef std::shared_ptr<wss_t> wss_ptr; 
typedef std::weak_ptr<wss_t> wss_weak_ptr; 
typedef std::shared_ptr<wss_clt_t> wss_clt_ptr; 
typedef std::weak_ptr<wss_clt_t> wss_clt_weak_ptr; 
#endif
typedef XIdleService<Server> This;
  public:
	XIdleService(Server &srv)
		: server_(srv), service_(), service_work_(service_), timing_wheel_timer_(service_)
	{
	}

	void start()
	{
#if XSERVER_PROTOTYPE_TCP
		worker_wheel_list_.resize(server_.keepalive());
		connector_wheel_list_.resize(server_.keepalive());
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
		ws_wheel_list_.resize(server_.keepalive());
		ws_clt_wheel_list_.resize(server_.keepalive());
#endif //
		post_timing_wheel();
		thread_ = boost::shared_ptr<boost::thread>(new boost::thread(
			boost::bind(&boost::asio::io_service::run, &service_)));
	}

	void stop()
	{
		boost::system::error_code ec;
		timing_wheel_timer_.cancel(ec);
		service_.stop();
		thread_->join();
		thread_.reset();
		service_.reset();
#if XSERVER_PROTOTYPE_TCP
		worker_wheel_list_.clear();
		connector_wheel_list_.clear();
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
		ws_wheel_list_.clear();
		ws_clt_wheel_list_.clear();
#endif //
	}

#if XSERVER_PROTOTYPE_TCP

	void add(tcp_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_worker, this, peer_ptr));
	}

	void add(tcp_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_connector, this, peer_ptr));
	}

	void active(tcp_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_active_worker, this, peer_ptr));
	}

	void active(tcp_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_active_connector, this, peer_ptr));
	}

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET

	void add(ws_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_ws, this, peer_ptr));
	}

	void add(ws_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_ws_client, this, peer_ptr));
	}

	void active(ws_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_active_ws, this, peer_ptr));
	}

	void active(ws_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_active_ws_client, this, peer_ptr));
	}

#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	void add(wss_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_wss, this, peer_ptr));
	}

	void add(wss_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_add_wss_client, this, peer_ptr));
	}

	void active(wss_ptr peer_ptr)
	{
		/*service_.post(boost::bind(&on_active_wss
		, this, peer_ptr));*/
	}

	void active(wss_clt_ptr peer_ptr)
	{
		service_.post(boost::bind(&This::on_active_wss_client, this, peer_ptr));
	}

#endif //

	template <typename F>
	void Post(F f)
	{
		service_.post(f);
	}

  private:
  #if XSERVER_PROTOTYPE_TCP

	void on_add_worker(tcp_ptr peer_ptr)
	{
		tcp_entry_ptr entry_ptr(new tcp_entry(peer_ptr));
		worker_wheel_list_.back().insert(entry_ptr);
		tcp_entry_weak_ptr entry_weak_ptr(entry_ptr);
		peer_ptr->set_context(entry_weak_ptr);
	}

	void on_add_connector(tcp_clt_ptr peer_ptr)
	{
		tcp_clt_entry_ptr entry_ptr(new tcp_clt_entry(peer_ptr));
		connector_wheel_list_.back().insert(entry_ptr);
		tcp_clt_entry_weak_ptr entry_weak_ptr(entry_ptr);
		peer_ptr->set_context(entry_weak_ptr);
	}

	void on_active_worker(tcp_ptr peer_ptr)
	{
		BOOST_ASSERT(!peer_ptr->context().empty());
		tcp_entry_weak_ptr entry_weak_ptr(boost::any_cast<tcp_entry_weak_ptr>(peer_ptr->context()));
		tcp_entry_ptr entry_ptr(entry_weak_ptr.lock());
		if (entry_ptr)
		{
			worker_wheel_list_.back().insert(entry_ptr);
		}
	}

	void on_active_connector(tcp_clt_ptr peer_ptr)
	{
		BOOST_ASSERT(!peer_ptr->context().empty());
		tcp_clt_entry_weak_ptr entry_weak_ptr(boost::any_cast<tcp_clt_entry_weak_ptr>(peer_ptr->context()));
		tcp_clt_entry_ptr entry_ptr(entry_weak_ptr.lock());
		if (entry_ptr)
		{
			connector_wheel_list_.back().insert(entry_ptr);
		}
	}

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET

	void on_add_ws(ws_ptr peer_ptr)
	{
		ws_entry_ptr entry_ptr(new ws_entry(peer_ptr));
		ws_wheel_list_.back().insert(entry_ptr);
		ws_entry_weak_ptr entry_weak_ptr(entry_ptr);
		peer_ptr->set_context(entry_weak_ptr);
	}

	void on_add_ws_client(ws_clt_ptr peer_ptr)
	{
		ws_clt_entry_ptr entry_ptr(new ws_clt_entry(peer_ptr));
		ws_clt_wheel_list_.back().insert(entry_ptr);
		ws_clt_entry_weak_ptr entry_weak_ptr(entry_ptr);
		peer_ptr->set_context(entry_weak_ptr);
	}

	void on_active_ws(ws_ptr peer_ptr)
	{
		BOOST_ASSERT(!peer_ptr->context().empty());
		ws_entry_weak_ptr entry_weak_ptr(boost::any_cast<ws_entry_weak_ptr>(peer_ptr->context()));
		ws_entry_ptr entry_ptr(entry_weak_ptr.lock());
		if (entry_ptr)
		{
			ws_wheel_list_.back().insert(entry_ptr);
		}
	}

	void on_active_ws_client(ws_clt_ptr peer_ptr)
	{
		BOOST_ASSERT(!peer_ptr->context().empty());
		ws_clt_entry_weak_ptr entry_weak_ptr(boost::any_cast<ws_clt_entry_weak_ptr>(peer_ptr->context()));
		ws_clt_entry_ptr entry_ptr(entry_weak_ptr.lock());
		if (entry_ptr)
		{
			ws_clt_wheel_list_.back().insert(entry_ptr);
		}
	}

#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	void on_add_wss(wss_ptr peer_ptr)
	{
		/*ws_entry_ptr entry_ptr(new ws_entry(peer_ptr));
	ws_wheel_list_.back().insert(entry_ptr);
	ws_entry_weak_ptr entry_weak_ptr(entry_ptr);
	peer_ptr->set_context(entry_weak_ptr);*/
	}

	void on_add_wss_client(wss_clt_ptr peer_ptr)
	{
		/*ws_clt_entry_ptr entry_ptr(new ws_clt_entry(peer_ptr));
	ws_clt_wheel_list_.back().insert(entry_ptr);
	ws_clt_entry_weak_ptr entry_weak_ptr(entry_ptr);
	peer_ptr->set_context(entry_weak_ptr);*/
	}

	void on_active_wss(wss_ptr peer_ptr)
	{
		/*BOOST_ASSERT(!peer_ptr->context().empty());
	ws_entry_weak_ptr entry_weak_ptr(boost::any_cast<ws_entry_weak_ptr>(peer_ptr->context()));
	ws_entry_ptr entry_ptr(entry_weak_ptr.lock());
	if (entry_ptr) {
		ws_wheel_list_.back().insert(entry_ptr);
	}*/
	}

	void on_active_wss_client(wss_clt_ptr peer_ptr)
	{
		/*BOOST_ASSERT(!peer_ptr->context().empty());
	ws_clt_entry_weak_ptr entry_weak_ptr(boost::any_cast<ws_clt_entry_weak_ptr>(peer_ptr->context()));
	ws_clt_entry_ptr entry_ptr(entry_weak_ptr.lock());
	if (entry_ptr) {
		ws_clt_wheel_list_.back().insert(entry_ptr);
	}*/
	}

#endif //

	void post_timing_wheel()
	{
		boost::system::error_code ec;
		timing_wheel_timer_.cancel(ec);
		timing_wheel_timer_.expires_from_now(boost::posix_time::seconds(1));
		timing_wheel_timer_.async_wait(boost::bind(&This::on_timing_wheel, this, boost::asio::placeholders::error));
	}

	void on_timing_wheel(const boost::system::error_code &ec)
	{
		if (!ec)
		{
#if XSERVER_PROTOTYPE_TCP
			worker_wheel_list_.push_back(tcp_entry_bucket());
			BOOST_ASSERT(worker_wheel_list_.size() == server_.keepalive());
			connector_wheel_list_.push_back(tcp_clt_entry_bucket());
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
			ws_wheel_list_.push_back(ws_entry_bucket());
			BOOST_ASSERT(ws_wheel_list_.size() == server_.keepalive());
			ws_clt_wheel_list_.push_back(ws_clt_entry_bucket());
#endif // 
	   //ping middle of wheel
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
			ws_entry_bucket &ws_list = ws_wheel_list_[ws_wheel_list_.size() / 2];
			for (typename ws_entry_bucket::iterator it = ws_list.begin(); it != ws_list.end(); ++it)
			{
				(*it)->ping();
			}
			ws_clt_entry_bucket &ws_clt_list = ws_clt_wheel_list_[ws_clt_wheel_list_.size() / 2];
			for (typename ws_clt_entry_bucket::iterator it = ws_clt_list.begin(); it != ws_clt_list.end(); ++it)
			{
				(*it)->ping();
			}
#endif //
		}
		post_timing_wheel();
	}

  private:
	Server &server_;
	boost::asio::io_service service_;
	//使用work，即使没有异步io的情况下，也能保证io_service继续工作
	boost::asio::io_service::work service_work_;
	boost::shared_ptr<boost::thread> thread_;
	boost::asio::deadline_timer timing_wheel_timer_;
	//
	template <class entry>
	struct x_idle_entry : private boost::noncopyable
	{
	  public:
		explicit x_idle_entry(const std::weak_ptr<entry> &entry_ptr)
			: entry_ptr_(entry_ptr)
		{
		}

		~x_idle_entry()
		{
			std::shared_ptr<entry> entry_ptr = entry_ptr_.lock();
			if (entry_ptr)
			{
				LOG4I("XPeer%d) HAS BEEN KICKED", entry_ptr->id());
				entry_ptr->close();
			}
		}

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET || XSERVER_PROTOTYPE_SSL_WEBSOCKET
		void ping()
		{
			std::shared_ptr<entry> entry_ptr = entry_ptr_.lock();
			if (entry_ptr)
			{
				LOG4I("XPeer%d) PING...", entry_ptr->id());
				entry_ptr->ping();
			}
		}
#endif //
	  private:
		std::weak_ptr<entry> entry_ptr_;
	};
#if XSERVER_PROTOTYPE_TCP
	typedef x_idle_entry<tcp_t> tcp_entry;
	typedef std::shared_ptr<tcp_entry> tcp_entry_ptr;
	typedef std::weak_ptr<tcp_entry> tcp_entry_weak_ptr;
	typedef boost::unordered_set<tcp_entry_ptr> tcp_entry_bucket;
	typedef boost::circular_buffer<tcp_entry_bucket> tcp_entry_bucket_list;
	tcp_entry_bucket_list worker_wheel_list_;
	typedef x_idle_entry<tcp_clt_t> tcp_clt_entry;
	typedef std::shared_ptr<tcp_clt_entry> tcp_clt_entry_ptr;
	typedef std::weak_ptr<tcp_clt_entry> tcp_clt_entry_weak_ptr;
	typedef boost::unordered_set<tcp_clt_entry_ptr> tcp_clt_entry_bucket;
	typedef boost::circular_buffer<tcp_clt_entry_bucket> tcp_clt_entry_bucket_list;
	tcp_clt_entry_bucket_list connector_wheel_list_;
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
	typedef x_idle_entry<ws_t> ws_entry;
	typedef std::shared_ptr<ws_entry> ws_entry_ptr;
	typedef std::weak_ptr<ws_entry> ws_entry_weak_ptr;
	typedef boost::unordered_set<ws_entry_ptr> ws_entry_bucket;
	typedef boost::circular_buffer<ws_entry_bucket> ws_entry_bucket_list;
	ws_entry_bucket_list ws_wheel_list_;
	typedef x_idle_entry<ws_clt_t> ws_clt_entry;
	typedef std::shared_ptr<ws_clt_entry> ws_clt_entry_ptr;
	typedef std::weak_ptr<ws_clt_entry> ws_clt_entry_weak_ptr;
	typedef boost::unordered_set<ws_clt_entry_ptr> ws_clt_entry_bucket;
	typedef boost::circular_buffer<ws_clt_entry_bucket> ws_clt_entry_bucket_list;
	ws_clt_entry_bucket_list ws_clt_wheel_list_;
#endif //
};

}

#endif //__H_XNET_XIDLESERVICE_H_
