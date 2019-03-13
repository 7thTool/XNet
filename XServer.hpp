#ifndef __H_XNET_XSERVER_HPP__
#define __H_XNET_XSERVER_HPP__

#include "XType.hpp"
#include "XIOService.hpp"
#include "XWorkService.hpp"
#include "XIdleService.hpp"
#include "XSocket.hpp"
#include "XBeast.hpp"
#include "XService.hpp"

namespace XNet {

template<class T, class TListener>
class XServerT : public XServiceT<T>
{
	typedef XServiceT<T> base;
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
	class Listener
	{
	public:
	#if XSERVER_PROTOTYPE_TCP
		void on_tcp_accept(tcp_ptr peer_ptr) {}
		void on_tcp_clt_connect(tcp_clt_ptr peer_ptr) {}
		void on_tcp_read(tcp_ptr peer_ptr, XBuffer &buffer) {}
		void on_tcp_write(tcp_ptr peer_ptr, XBuffer &buffer) {}
		void on_tcp_close(tcp_t *peer_ptr) {}
		void on_tcp_clt_read(tcp_clt_ptr peer_ptr, XBuffer &buffer) {}
		void on_tcp_clt_write(tcp_clt_ptr peer_ptr, XBuffer &buffer) {}
		void on_tcp_clt_close(tcp_clt_t *peer_ptr) {}
	#endif //
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
		void on_http_accept(http_ptr peer_ptr) {}
		// This function produces an HTTP response for the given
		// request. The type of the response object depends on the
		// contents of the request, so the interface requires the
		// caller to pass a generic lambda for receiving the response.
		template <class Body, class Allocator>
		void on_http_read(http_ptr peer_ptr,
						boost::beast::string_view doc_root,
						boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req
						) {}
		void on_http_close(http_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTPS
		void on_https_accept(https_ptr peer_ptr) {}
		// This function produces an HTTP response for the given
		// request. The type of the response object depends on the
		// contents of the request, so the interface requires the
		// caller to pass a generic lambda for receiving the response.
		template <class Body, class Allocator>
		void on_https_read(https_ptr peer_ptr,
						boost::beast::string_view doc_root,
						boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req
						) {}
		void on_https_close(https_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
		template <class Body, class Allocator>
		void on_ws_preaccept(ws_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req) {}
		void on_ws_accept(ws_ptr peer_ptr) {}
		void on_ws_read(ws_ptr peer_ptr, std::string &buffer) {}
		void on_ws_write(ws_ptr peer_ptr, std::string &buffer) {}
		void on_ws_close(ws_t *peer_ptr) {}
		void on_ws_clt_connect(ws_clt_ptr peer_ptr) {}
		void on_ws_clt_read(ws_clt_ptr peer_ptr, std::string &buffer) {}
		void on_ws_clt_write(ws_clt_ptr peer_ptr, std::string &buffer) {}
		void on_ws_clt_close(ws_clt_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
		template <class Body, class Allocator>
		void on_wss_preaccept(wss_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req) {}
		void on_wss_accept(wss_ptr peer_ptr) {}
		void on_wss_read(wss_ptr peer_ptr, std::string &buffer) {}
		void on_wss_write(wss_ptr peer_ptr, std::string &buffer) {}
		void on_wss_close(wss_t *peer_ptr) {}
		void on_wss_clt_connect(wss_clt_ptr peer_ptr) {}
		void on_wss_clt_read(wss_clt_ptr peer_ptr, std::string &buffer) {}
		void on_wss_clt_write(wss_clt_ptr peer_ptr, std::string &buffer) {}
		void on_wss_clt_close(wss_clt_t *peer_ptr) {}
	#endif
	};
public:
	XServerT(TListener* listener = nullptr)
		: base(), listener_(listener)
	{
	}

	~XServerT()
	{
	}

	const char* name() 
	{
		return "xserver";
	}

	inline void set_listener(TListener* listener) { listener_ = listener; }
	inline TListener* listener() { return listener_; }

#if XSERVER_PROTOTYPE_TCP
	void on_io_accept(tcp_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_tcp_accept(peer_ptr);
	}

	void on_io_connect(tcp_clt_ptr peer_ptr)
	{
		base::on_io_connect(peer_ptr);
		listener_->on_tcp_clt_connect(peer_ptr);
	}

	void on_io_read(tcp_ptr peer_ptr, XBuffer &buffer)
	{
		listener_->on_tcp_read(peer_ptr, buffer);
	}

	void on_io_write(tcp_ptr peer_ptr, XBuffer &buffer)
	{
		listener_->on_tcp_write(peer_ptr, buffer);
	}

	void on_io_close(tcp_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_tcp_close(peer_ptr);
	}

	void on_io_read(tcp_clt_ptr peer_ptr, XBuffer &buffer)
	{
		listener_->on_tcp_clt_read(peer_ptr, buffer);
	}

	void on_io_write(tcp_clt_ptr peer_ptr, XBuffer &buffer)
	{
		listener_->on_tcp_clt_write(peer_ptr, buffer);
	}

	void on_io_close(tcp_clt_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_tcp_clt_close(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
	void on_io_accept(http_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_http_accept(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTPS
	void on_io_accept(https_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_https_accept(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
	void on_io_accept(ws_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_ws_accept(peer_ptr);
	}

	void on_io_connect(ws_clt_ptr peer_ptr)
	{
		base::on_io_connect(peer_ptr);
		listener_->on_ws_clt_connect(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
	void on_io_accept(wss_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_wss_accept(peer_ptr);
	}

	void on_io_connect(wss_clt_ptr peer_ptr)
	{
		base::on_io_connect(peer_ptr);
		listener_->on_wss_clt_connect(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template <class Body, class Allocator>
	void
	on_io_read(http_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req
			   )
	{
		listener_->on_http_read(peer_ptr, doc_root, std::move(req));
	}

	void on_io_close(http_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_http_close(peer_ptr);
	}
#endif //
#if XSERVER_PROTOTYPE_HTTPS
	// This function produces an HTTP response for the given
	// request. The type of the response object depends on the
	// contents of the request, so the interface requires the
	// caller to pass a generic lambda for receiving the response.
	template <class Body, class Allocator>
	void
	on_io_read(https_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req
			   )
	{
		listener_->on_https_read(peer_ptr, doc_root, std::move(req));
	}

	void on_io_close(https_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_https_close(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
	template <class Body, class Allocator>
	void
	on_io_preaccept(ws_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{
		listener_->on_ws_preaccept(peer_ptr, std::move(req));
	}

	void on_io_read(ws_ptr peer_ptr, std::string &buffer)
	{
		listener_->on_ws_read(peer_ptr, buffer);
	}

	void on_io_write(ws_ptr peer_ptr, std::string &buffer)
	{
		listener_->on_ws_write(peer_ptr, buffer);
	}

	void on_io_close(ws_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_ws_close(peer_ptr);
	}

	void on_io_read(ws_clt_ptr peer_ptr, std::string &buffer)
	{
		base::on_io_read(peer_ptr, buffer);
		listener_->on_ws_clt_read(peer_ptr, buffer);
	}

	void on_io_write(ws_clt_ptr peer_ptr, std::string &buffer)
	{
		base::on_io_write(peer_ptr, buffer);
		listener_->on_ws_clt_write(peer_ptr, buffer);
	}

	void on_io_close(ws_clt_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_ws_clt_close(peer_ptr);
	}
#endif //

#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
	template <class Body, class Allocator>
	void
	on_io_preaccept(wss_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req)
	{
		listener_->on_wss_preaccept(peer_ptr, std::move(req));
	}

	void on_io_read(wss_ptr peer_ptr, std::string &buffer)
	{
		listener_->on_wss_read(peer_ptr, buffer);
	}

	void on_io_write(wss_ptr peer_ptr, std::string &buffer)
	{
		listener_->on_wss_write(peer_ptr, buffer);
	}

	void on_io_close(wss_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_wss_close(peer_ptr);
	}

	void on_io_read(wss_clt_ptr peer_ptr, std::string &buffer)
	{
		base::on_io_read(peer_ptr, buffer);
		listener_->on_wss_clt_read(peer_ptr, buffer);
	}

	void on_io_write(wss_clt_ptr peer_ptr, std::string &buffer)
	{
		base::on_io_write(peer_ptr, buffer);
		listener_->on_wss_clt_write(peer_ptr, buffer);
	}

	void on_io_close(wss_clt_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_wss_clt_close(peer_ptr);
	}
#endif //

protected:
	TListener* listener_;
};

}

#endif //__H_XNET_XSERVER_HPP__
