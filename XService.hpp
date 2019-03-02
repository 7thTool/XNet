#ifndef __H_XNET_XSERVICE_HPP__
#define __H_XNET_XSERVICE_HPP__

#include "XType.hpp"
#include "XIOService.hpp"
#include "XWorkService.hpp"
#include "XIdleService.hpp"
#include "XSocket.hpp"
#include "XBeast.hpp"

namespace XNet {

template <class T>
class XServiceT : private boost::noncopyable
{
public:
#if XSERVER_PROTOTYPE_TCP 
typedef tcp_peer_session<T> tcp_t; 
typedef tcp_client_session<T> tcp_clt_t; 
typedef std::shared_ptr<tcp_t> tcp_ptr; 
typedef std::weak_ptr<tcp_t> tcp_weak_ptr; 
typedef std::shared_ptr<tcp_clt_t> tcp_clt_ptr; 
typedef std::weak_ptr<tcp_clt_t> tcp_clt_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_HTTPS
typedef plain_http_session<T> http_t; 
typedef std::shared_ptr<http_t> http_ptr; 
typedef std::weak_ptr<http_t> http_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTPS
typedef detect_session<T> detect_t; 
typedef std::shared_ptr<detect_t> detect_ptr; 
typedef ssl_http_session<T> https_t; 
typedef std::shared_ptr<https_t> https_ptr; 
typedef std::weak_ptr<https_t> https_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET 
typedef plain_websocket_session<T> ws_t; 
typedef plain_websocket_client_session<T> ws_clt_t; 
typedef std::shared_ptr<ws_t> ws_ptr; 
typedef std::weak_ptr<ws_t> ws_weak_ptr; 
typedef std::shared_ptr<ws_clt_t> ws_clt_ptr; 
typedef std::weak_ptr<ws_clt_t> ws_clt_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET 
typedef ssl_websocket_session<T> wss_t; 
typedef ssl_websocket_client_session<T> wss_clt_t; 
typedef std::shared_ptr<wss_t> wss_ptr; 
typedef std::weak_ptr<wss_t> wss_weak_ptr; 
typedef std::shared_ptr<wss_clt_t> wss_clt_ptr; 
typedef std::weak_ptr<wss_clt_t> wss_clt_weak_ptr; 
#endif
  public:
	XServiceT()
		: keepalive_(60), max_buffer_size_(4096), log_directory_("./log/"), root_directory_("./www/"), io_thread_num_(0), io_service_()
#if XSERVER_PROTOTYPE_SSL
		, ssl_directory_("./")
		, io_ssl_context_(boost::asio::ssl::context::sslv23)
#endif
		, stop_flag_(true), peer_id_(0)
	{
	}

	~XServiceT()
	{
	}

	const char* name() 
	{
		return "xserver";
	}

	inline void set_keepalive(size_t val) { keepalive_ = val; }
	inline const size_t& keepalive() { return keepalive_; }

	inline void set_max_buffer_size(size_t val) { max_buffer_size_ = val; }
	inline const size_t& max_buffer_size() { return max_buffer_size_; }

	inline void set_log_directory(const std::string& val) { log_directory_ = val; }
	inline const std::string& log_directory() { return log_directory_; }

	inline void set_root_directory(const std::string& val) { root_directory_ = val; }
	inline const std::string& root_directory() { return root_directory_; }

#if XSERVER_PROTOTYPE_SSL
	inline void set_ssl_directory(const std::string& val) { ssl_directory_ = val; }
	inline const std::string& ssl_directory() { return ssl_directory_; }

	void load_server_certificate(boost::asio::ssl::context& ctx)
	{
		//
	}
#endif

	inline bool is_run() { return !stop_flag_; }

	bool start(int io_thread)
	{
		bool expected = true;
		if (!stop_flag_.compare_exchange_strong(expected, false))
		{
			return true;
		}

		T* pT = static_cast<T*>(this);

		boost::system::error_code ec;

		io_thread_num_ = io_thread + 1;
		peer_id_ = std::max<>(io_thread_num_, size_t(1));

		//size_t i;

		srand((unsigned int)time(0));

		//std::string logfile = log_directory_ + pT->name();
		//XLogger::instance().init(logfile);

		io_service_ = std::make_shared<XIOService<T>>(*static_cast<T*>(this));

#if XSERVER_PROTOTYPE_SSL

		// This holds the self-signed certificate used by the server
		pT->load_server_certificate(io_ssl_context_);
		
#endif //

		LOG4I("XSERVER t=%d v=%s", XSERVER_PROTOTYPE, XSERVER_VERSION);
		LOG4I("XSERVER starting io_thread=%d", io_thread);

		io_service_->Start(io_thread_num_);

		//LOG4I("XSERVER started");

		return true;
	}

	void stop()
	{
		bool expected = false;
		if (!stop_flag_.compare_exchange_strong(expected, true))
		{
			return;
		}

		//size_t i;
		boost::system::error_code ec;

		LOG4I("XSERVER stoping");

		//acceptor_.reset();

		io_service_->Stop();

		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_.clear();
			//lock.unlock();
		}

		io_service_.reset();
		
		LOG4I("XSERVER stoped");
	}

	void on_io_init(const size_t peer)
	{
		LOG4I("XSERVER on_io_init %d", peer);
	}

	void on_io_term(const size_t peer)
	{
		LOG4I(" on_io_term %d", peer);
	}

	template <typename F>
	inline void post_io_callback(size_t peer, F f)
	{
		io_service_->Post(get_io_index(PEER_ID(peer)), f);
	}
	inline boost::asio::deadline_timer *create_io_timer(size_t peer)
	{
		return io_service_->CreateTimer(get_io_index(PEER_ID(peer)));
	}
	template <typename F>
	inline void post_io_timer(boost::asio::deadline_timer *timer_ptr, size_t millis, F f)
	{
		io_service_->PostTimer(timer_ptr, millis, f);
	}
	inline void kill_io_timer(boost::asio::deadline_timer *timer_ptr)
	{
		io_service_->KillTimer(timer_ptr);
	}

	template<typename Ty>
	std::shared_ptr<Ty> connect(const std::string& addr, const unsigned short port
		, size_t timeout
		, const size_t io_channel = 0)
	{
		std::shared_ptr<Ty> peer_ptr;
		if (!is_run()) {
			return peer_ptr;
		}

		LOG4I("XSERVER connecting addr=%s port=%d type=%s", addr.c_str(), port, typeid(Ty).name());

		size_t peer_id = 0;
		if (io_channel != (x_size_t)-1)
		{
			peer_id = new_io_channel_peer_id(io_channel);
		}
		if (peer_id == 0)
		{
			peer_id = new_peer_id();
		}
		boost::asio::ip::tcp::socket socket(io_service_->get_service(get_io_index(peer_id)));
		peer_ptr = std::make_shared<Ty>(*static_cast<T*>(this), peer_id, std::move(socket));
		if (timeout) {
			peer_ptr->set_connect_timeout(timeout);
		}
		peer_ptr->run(addr, tostr<unsigned short>(port));
		return peer_ptr;
	}

	bool listen(const unsigned short port, const size_t type = XSERVER_DEFAULT)
	{
		if (!is_run())
		{
			return false;
		}

		LOG4I("XSERVER listen port=%d type=%d", port, type);

		boost::system::error_code ec;
		boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), port);
		std::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor = std::make_shared<boost::asio::ip::tcp::acceptor>(io_service_->get_service(get_accept_index()));
		acceptor->open(ep.protocol());
		acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		acceptor->bind(ep);
		acceptor->listen(boost::asio::socket_base::max_listen_connections, ec);
		post_accept(acceptor, type);
		return true;
	}

	void close(const size_t peer)
	{
		if (!is_run())
		{
			return;
		}

		LOG4I("XServer CLOSE PEER(%d,%d)", PEER_TYPE(peer), PEER_ID(peer));

		boost::shared_lock<boost::shared_mutex> lock(peer_mutex_);
		boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer);
		if (it != peer_map_.end())
		{
			if (0)
			{
				//
			}
#if XSERVER_PROTOTYPE_WEBSOCKET
			else if (PEER_TYPE(peer) == PEER_TYPE_WEBSOCKET_CLIENT)
			{
				ws_clt_ptr peer_ptr;
				{
					ws_clt_weak_ptr peer_weak_ptr = boost::any_cast<ws_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
#elif XSERVER_PROTOTYPE_SSL_WEBSOCKET
			else if (PEER_TYPE(peer) == PEER_TYPE_SSL_WEBSOCKET_CLIENT)
			{
				wss_clt_ptr peer_ptr;
				{
					wss_clt_weak_ptr peer_weak_ptr = boost::any_cast<wss_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
			else if (PEER_TYPE(peer) == PEER_TYPE_WEBSOCKET)
			{
				ws_ptr peer_ptr;
				{
					ws_weak_ptr peer_weak_ptr = boost::any_cast<ws_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
#elif XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
			else if (PEER_TYPE(peer) == PEER_TYPE_SSL_WEBSOCKET)
			{
				wss_ptr peer_ptr;
				{
					wss_weak_ptr peer_weak_ptr = boost::any_cast<wss_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
#else
			else if (PEER_TYPE(peer) == PEER_TYPE_TCP_CLIENT)
			{
				tcp_clt_ptr peer_ptr;
				{
					tcp_clt_weak_ptr peer_weak_ptr = boost::any_cast<tcp_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
			else if (PEER_TYPE(peer) == PEER_TYPE_TCP)
			{
				tcp_ptr peer_ptr;
				{
					tcp_weak_ptr peer_weak_ptr = boost::any_cast<tcp_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					peer_ptr->close();
				}
			}
#endif //
			else
			{
			}
			peer_map_.erase(it);
		}
	}

	bool post_packet(const size_t peer, const char *buf, const size_t len)
	{
		if (!is_run())
		{
			return false;
		}
		bool rlt = false;
		boost::shared_lock<boost::shared_mutex> lock(peer_mutex_);
		boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer);
		if (it != peer_map_.end())
		{
			size_t peer_type = PEER_TYPE(peer);
			switch (peer_type)
			{
#if XSERVER_PROTOTYPE_WEBSOCKET
			case PEER_TYPE_WEBSOCKET_CLIENT:
			{
				ws_clt_ptr peer_ptr;
				{
					ws_clt_weak_ptr peer_weak_ptr = boost::any_cast<ws_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_SSL_WEBSOCKET
			case PEER_TYPE_SSL_WEBSOCKET_CLIENT:
			{
				wss_clt_ptr peer_ptr;
				{
					wss_clt_weak_ptr peer_weak_ptr = boost::any_cast<wss_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
			case PEER_TYPE_WEBSOCKET:
			{
				ws_ptr peer_ptr;
				{
					ws_weak_ptr peer_weak_ptr = boost::any_cast<ws_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
			case PEER_TYPE_SSL_WEBSOCKET:
			{
				wss_ptr peer_ptr;
				{
					wss_weak_ptr peer_weak_ptr = boost::any_cast<wss_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_TCP
			case PEER_TYPE_TCP_CLIENT:
			{
				tcp_clt_ptr peer_ptr;
				{
					tcp_clt_weak_ptr peer_weak_ptr = boost::any_cast<tcp_clt_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
			case PEER_TYPE_TCP:
			{
				tcp_ptr peer_ptr;
				{
					tcp_weak_ptr peer_weak_ptr = boost::any_cast<tcp_weak_ptr>(it->second);
					peer_ptr = peer_weak_ptr.lock();
				}
				if (peer_ptr)
				{
					if (peer_ptr->is_open())
					{
						peer_ptr->do_write(buf, len);
						rlt = true;
					}
				}
			}
			break;
#endif //
			default:
			{
			}
			break;
			}
		}
		return rlt;
	}
	//void broadcast(const char* buf, const size_t len);

#if XSERVER_PROTOTYPE_TCP
	//int parse_buffer(tcp_ptr peer_ptr, const char* buf, const size_t len);
	//int parse_buffer(tcp_clt_ptr peer_ptr, const char* buf, const size_t len);

	void on_io_accept(tcp_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = tcp_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}

	void on_io_connect(tcp_clt_ptr peer_ptr)
	{
		// 	if (err && err[0]) {
		// 		handler_->handle_io_connect(peer_ptr->id(), XSERVER_TCP, err);
		// 	}
		// 	else {
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = tcp_clt_weak_ptr(peer_ptr);
			//lock.unlock();
		}
		//	}
	}

	void on_io_read(tcp_ptr peer_ptr, XBuffer &buffer)
	{
	}

	void on_io_write(tcp_ptr peer_ptr, XBuffer &buffer)
	{
	}

	void on_io_close(tcp_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("tcp_t(%d) has been closed", peer_ptr->id());
		//}
	}

	void on_io_read(tcp_clt_ptr peer_ptr, XBuffer &buffer)
	{
	}

	void on_io_write(tcp_clt_ptr peer_ptr, XBuffer &buffer)
	{
	}

	void on_io_close(tcp_clt_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("tcp_client_session(%d) has been closed", peer_ptr->id());
		//}
	}
#endif //

#if XSERVER_PROTOTYPE_HTTPS
	//detect_ptr检测完成之后会变成http_ptr或https_ptr
	//http_ptr和https_ptr分别可以升级为ws_ptr和wss_ptr
	void on_io_accept(detect_ptr peer_ptr, const int_t type)
	{
		
	}
#endif

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
	void on_io_accept(http_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = http_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}

	void on_io_upgrade(ws_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_.erase(MAKE_PEER_ID(PEER_TYPE_HTTP, peer_ptr->id()));
			peer_map_[peer_ptr->id()] = ws_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}
#endif

#if XSERVER_PROTOTYPE_HTTPS
	void on_io_accept(https_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = https_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}

	void on_io_upgrade(wss_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_.erase(MAKE_PEER_ID(PEER_TYPE_HTTPS, peer_ptr->id()));
			peer_map_[peer_ptr->id()] = wss_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
	void on_io_accept(ws_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = ws_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}

	void on_io_connect(ws_clt_ptr peer_ptr)
	{
		// 	if (err && err[0]) {
		// 		handler_->handle_io_connect(peer_ptr->id(), XSERVER_WEBSOCKET, err);
		// 	}
		// 	else {
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = ws_clt_weak_ptr(peer_ptr);
			//lock.unlock();
		}
		//	}
	}
#endif //
#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	void on_io_accept(wss_ptr peer_ptr)
	{
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = wss_weak_ptr(peer_ptr);
			//lock.unlock();
		}
	}

	void on_io_connect(wss_clt_ptr peer_ptr)
	{
		// 	if (err && err[0]) {
		// 		handler_->handle_io_connect(peer_ptr->id(), XSERVER_SSL_WEBSOCKET, err);
		// 	}
		// 	else {
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			peer_map_[peer_ptr->id()] = wss_clt_weak_ptr(peer_ptr);
			//lock.unlock();
		}
		//	}
	}

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template <class Body, class Allocator,
			  class Send>
	void
	on_io_read(http_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
			   Send &&send)
	{
		//
	}

	void on_io_close(http_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("http_t(%d) has been closed", peer_ptr->id());
		//}
	}
#endif
#if XSERVER_PROTOTYPE_HTTPS
	template <class Body, class Allocator,
			  class Send>
	void
	on_io_read(https_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
			   Send &&send)
	{
		//
	}

	void on_io_close(https_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("https_t(%d) has been closed", peer_ptr->id());
		//}
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
	template <class Body, class Allocator>
	void
	on_io_preaccept(ws_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{

	}
	
	void on_io_activity(ws_ptr peer_ptr)
	{
	}

	void on_io_read(ws_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_write(ws_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_close(ws_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("ws_t(%d) has been closed", peer_ptr->id());
		//}
	}

	void on_io_activity(ws_clt_ptr peer_ptr)
	{
	}

	void on_io_read(ws_clt_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_write(ws_clt_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_close(ws_clt_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("ws_clt_t(%d) has been closed", peer_ptr->id());
		//}
	}
#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	template <class Body, class Allocator>
	void
	on_io_preaccept(wss_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{

	}
	
	void on_io_activity(wss_ptr peer_ptr)
	{
	}

	void on_io_read(wss_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_write(wss_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_close(wss_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("wss_t(%d) has been closed", peer_ptr->id());
		//}
	}

	void on_io_activity(wss_clt_ptr peer_ptr)
	{
	}

	void on_io_read(wss_clt_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_write(wss_clt_ptr peer_ptr, std::string &buffer)
	{
	}

	void on_io_close(wss_clt_t *peer_ptr)
	{
		bool bfind = false;
		{
			boost::unique_lock<boost::shared_mutex> lock(peer_mutex_);
			boost::unordered_map<size_t, boost::any>::iterator it = peer_map_.find(peer_ptr->id());
			if (it != peer_map_.end())
			{
				bfind = true;
				peer_map_.erase(it);
				lock.unlock();
			}
			//lock.unlock();
		}
		//if (bfind) {
		LOG4I("wss_clt_t(%d) has been closed", peer_ptr->id());
		//}
	}
#endif //

  protected:
	size_t new_peer_id()
	{
		//它们对比变量的值和期待的值是否一致，
		//如果是，则替换为用户指定的一个新的数值。
		//如果不是，则将变量的值和期待的值交换。
		size_t expected = PEER_ID_MASK; //PEER_ID_MASK==MAX_PEER_ID
		peer_id_.compare_exchange_weak(expected, std::max<>(io_thread_num_, (size_t)1));
#ifdef _DEBUG
		size_t id = peer_id_;
		id = MAKE_PEER_ID(XSERVER_WEBSOCKET, id);
		size_t peer_type = PEER_TYPE(id);
		size_t peer_id = PEER_ID(id);
#endif //
		return peer_id_++;
	}

	size_t new_io_channel_peer_id(size_t channel)
	{
		channel %= (io_thread_num_ - 1);
		size_t peer_id = new_peer_id();
		//0是用于accept服务
		while (peer_id % (io_thread_num_ - 1) != channel)
			;
		return peer_id;
	}

	size_t get_accept_index()
	{
		return 0;
	}

	size_t get_io_index(size_t id)
	{
		BOOST_ASSERT(PEER_TYPE(id) == 0);
		//0是用于accept服务
		//size_t index = 1 + rand() % (io_service_->size()-1);
		return 1 + id % (io_thread_num_ - 1);
	}

	void post_accept(const std::shared_ptr<boost::asio::ip::tcp::acceptor> &acceptor, const int_t type)
	{
		if (!is_run())
		{
			return;
		}
		size_t peer_id = new_peer_id();
		//acceptor->async_accept(io_service_->get_socket(get_io_index(peer_id), peer_id),
		//	boost::bind(&on_accept, static_cast<T*>(this), boost::asio::placeholders::error, acceptor)
		//	);
		std::shared_ptr<boost::asio::ip::tcp::socket> socket = 
			std::make_shared<boost::asio::ip::tcp::socket>(io_service_->get_service(get_io_index(peer_id)));
		acceptor->async_accept(*socket, boost::bind(&T::on_accept, static_cast<T*>(this), boost::asio::placeholders::error, acceptor, type, socket, peer_id));
	}

	void on_accept(const boost::system::error_code &ec, const std::shared_ptr<boost::asio::ip::tcp::acceptor> &acceptor, const int_t type, const std::shared_ptr<boost::asio::ip::tcp::socket> &socket, const size_t peer_id)
	{
		if (ec)
		{
			fail(ec, "on_accept");
		}
		else
		{
			switch (type)
			{
#if XSERVER_PROTOTYPE_TCP
			case XSERVER_TCP:
			{
				tcp_ptr peer_ptr = std::make_shared<tcp_t>(*static_cast<T*>(this), peer_id, std::move(*socket));
				on_io_accept(peer_ptr);
				peer_ptr->run();
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_HTTPS
			case XSERVER_HTTP:
			case XSERVER_HTTPS:
			{
				detect_ptr peer_ptr = std::make_shared<detect_t>(*static_cast<T*>(this), peer_id, std::move(*socket),
																		  io_ssl_context_,
																		  root_directory_);
				on_io_accept(peer_ptr, type);
				peer_ptr->run();
			}
			break;
#elif XSERVER_PROTOTYPE_HTTP
			case XSERVER_HTTP:
			{
				http_ptr peer_ptr = std::make_shared<http_t>(*static_cast<T*>(this), peer_id, std::move(*socket),
																		  root_directory_);
				on_io_accept(peer_ptr);
				peer_ptr->run();
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_WEBSOCKET
			case XSERVER_WEBSOCKET:
			{
				ws_ptr peer_ptr = std::make_shared<ws_t>(*static_cast<T*>(this), peer_id, std::move(*socket));
				on_io_accept(peer_ptr);
				peer_ptr->run();
			}
			break;
#endif //
#if XSERVER_PROTOTYPE_SSL_WEBSOCKET
			case XSERVER_SSL_WEBSOCKET:
			{
				wss_ptr peer_ptr = std::make_shared<wss_t>(*static_cast<T*>(this), peer_id, std::move(*socket),
																			  io_ssl_context_);
				on_io_accept(peer_ptr);
				peer_ptr->run();
			}
			break;
#endif //
			default:
				BOOST_ASSERT(false);
				return;
				break;
			}
		}
		post_accept(acceptor, type);
	}

	size_t keepalive_;

	size_t max_buffer_size_;

	std::string log_directory_;
	std::string root_directory_;

	size_t io_thread_num_;

	std::atomic<bool> stop_flag_;

	std::shared_ptr<XIOService<T>> io_service_;

#if XSERVER_PROTOTYPE_SSL
	std::string ssl_directory_;
	boost::asio::ssl::context io_ssl_context_;
#endif

	std::atomic<size_t> peer_id_;
	boost::unordered_map<size_t, boost::any> peer_map_;
	boost::shared_mutex peer_mutex_;
};

}

#endif //__H_XNET_XSERVICE_HPP__
