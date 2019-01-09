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
	class Listener
	{
	public:
	#if XSERVER_PROTOTYPE_TCP
		void on_tcp_accept(xworker_ptr peer_ptr) {}
		void on_tcp_clt_connect(xconnector_ptr peer_ptr) {}
		void on_tcp_read(xworker_ptr peer_ptr, XRWBuffer &buffer) {}
		void on_tcp_write(xworker_ptr peer_ptr, XRWBuffer &buffer) {}
		void on_tcp_close(xworker_t *peer_ptr) {}
		void on_tcp_clt_read(xconnector_ptr peer_ptr, XRWBuffer &buffer) {}
		void on_tcp_clt_write(xconnector_ptr peer_ptr, XRWBuffer &buffer) {}
		void on_tcp_clt_close(xconnector_t *peer_ptr) {}
	#endif //
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
		void on_http_accept(http_ptr peer_ptr) {}
	#endif //
	#if XSERVER_PROTOTYPE_HTTPS
		void on_http_accept(http_ptr peer_ptr) {}
	#endif //
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
		void on_ws_accept(ws_ptr peer_ptr) {}
		void on_ws_clt_connect(ws_clt_ptr peer_ptr) {}
	#endif //
	#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
		void on_wss_accept(wss_ptr peer_ptr) {}
		void on_wss_clt_connect(wss_clt_ptr peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS
		// This function produces an HTTP response for the given
		// request. The type of the response object depends on the
		// contents of the request, so the interface requires the
		// caller to pass a generic lambda for receiving the response.
		template <class Body, class Allocator, class Send>
		void on_http_read(http_ptr peer_ptr,
						boost::beast::string_view doc_root,
						boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
						Send &&send) {}
		void on_http_close(http_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTPS
		// This function produces an HTTP response for the given
		// request. The type of the response object depends on the
		// contents of the request, so the interface requires the
		// caller to pass a generic lambda for receiving the response.
		template <class Body, class Allocator, class Send>
		void on_https_read(https_ptr peer_ptr,
						boost::beast::string_view doc_root,
						boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
						Send &&send) {}
		void on_https_close(https_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_WEBSOCKET
		template <class Body, class Allocator>
		void on_ws_preaccept(ws_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req) {}
		void on_ws_read(ws_ptr peer_ptr, std::string &buffer) {}
		void on_ws_write(ws_ptr peer_ptr, std::string &buffer) {}
		void on_ws_close(ws_t *peer_ptr) {}
		void on_ws_clt_read(ws_clt_ptr peer_ptr, std::string &buffer) {}
		void on_ws_clt_write(ws_clt_ptr peer_ptr, std::string &buffer) {}
		void on_ws_clt_close(ws_clt_t *peer_ptr) {}
	#endif
	#if XSERVER_PROTOTYPE_HTTPS || XSERVER_PROTOTYPE_SSL_WEBSOCKET
		template <class Body, class Allocator>
		void
		on_wss_preaccept(wss_ptr peer_ptr, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req) {}
		void on_wss_read(wss_ptr peer_ptr, std::string &buffer) {}
		void on_wss_write(wss_ptr peer_ptr, std::string &buffer) {}
		void on_wss_close(wss_t *peer_ptr) {}
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
	void on_io_accept(xworker_ptr peer_ptr)
	{
		base::on_io_accept(peer_ptr);
		listener_->on_tcp_accept(peer_ptr);
	}

	void on_io_connect(xconnector_ptr peer_ptr)
	{
		base::on_io_connect(peer_ptr);
		listener_->on_tcp_clt_connect(peer_ptr);
	}

	void on_io_read(xworker_ptr peer_ptr, XRWBuffer &buffer)
	{
		listener_->on_tcp_read(peer_ptr, buffer);
	}

	void on_io_write(xworker_ptr peer_ptr, XRWBuffer &buffer)
	{
		listener_->on_tcp_write(peer_ptr, buffer);
	}

	void on_io_close(xworker_t *peer_ptr)
	{
		base::on_io_close(peer_ptr);
		listener_->on_tcp_close(peer_ptr);
	}

	void on_io_read(xconnector_ptr peer_ptr, XRWBuffer &buffer)
	{
		listener_->on_tcp_clt_read(peer_ptr, buffer);
	}

	void on_io_write(xconnector_ptr peer_ptr, XRWBuffer &buffer)
	{
		listener_->on_tcp_clt_write(peer_ptr, buffer);
	}

	void on_io_close(xconnector_t *peer_ptr)
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
	template <class Body, class Allocator,
			  class Send>
	void
	on_io_read(http_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
			   Send &&send)
	{
		listener_->on_http_read(peer_ptr, doc_root, std::move(req), std::move(send));
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
	template <class Body, class Allocator,
			  class Send>
	void
	on_io_read(https_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
			   Send &&send)
	{
		listener_->on_https_read(peer_ptr, doc_root, std::move(req), std::move(send));
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
