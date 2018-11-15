#ifndef __H_XROUTER_H__
#define __H_XROUTER_H__

#include "XType.h"

namespace XNet {

template <class T>
class XRouterT : private boost::noncopyable
{
#if XSERVER_PROTOTYPE_TCP 
typedef XWorker<T> xworker_t; 
typedef XConnector<T> xconnector_t; 
typedef std::shared_ptr<xworker_t> xworker_ptr; 
typedef std::weak_ptr<xworker_t> xworker_weak_ptr; 
typedef std::shared_ptr<xconnector_t> xconnector_ptr; 
typedef std::weak_ptr<xconnector_t> xconnector_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP 
typedef detect_session<T> detect_t; 
typedef plain_http_session<T> http_t; 
typedef ssl_http_session<T> https_t; 
typedef std::shared_ptr<detect_t> detect_ptr; 
typedef std::shared_ptr<http_t> http_ptr; 
typedef std::weak_ptr<http_t> http_weak_ptr; 
typedef std::shared_ptr<https_t> https_ptr; 
typedef std::weak_ptr<https_t> https_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET 
typedef plain_websocket_session<T> ws_t; 
typedef plain_websocket_client_session<T> ws_clt_t; 
typedef std::shared_ptr<ws_t> ws_ptr; 
typedef std::weak_ptr<ws_t> ws_weak_ptr; 
typedef std::shared_ptr<ws_clt_t> ws_clt_ptr; 
typedef std::weak_ptr<ws_clt_t> ws_clt_weak_ptr; 
#endif 
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET 
typedef ssl_websocket_session<T> wss_t; 
typedef ssl_websocket_client_session<T> wss_clt_t; 
typedef std::shared_ptr<wss_t> wss_ptr; 
typedef std::weak_ptr<wss_t> wss_weak_ptr; 
typedef std::shared_ptr<wss_clt_t> wss_clt_ptr; 
typedef std::weak_ptr<wss_clt_t> wss_clt_weak_ptr; 
#endif
  public:
	XRouterT()
	{
	}

	~XRouterT()
	{
	}

	bool start(int io_thread, int work_thread)
	{
		return true;
	}

	void stop()
	{
		
	}

	void on_io_init(const size_t peer)
	{
		LOG4I("XSERVER on_io_init %d", peer);
	}

	void on_io_term(const size_t peer)
	{
		LOG4I(" on_io_term %d", peer);
	}

	void on_work_init(const size_t peer)
	{
		LOG4I("XSERVER on_work_init %d", peer);
	}

	void on_work_term(const size_t peer)
	{
		LOG4I("XSERVER on_work_term %d", peer);
	}

#if XSERVER_PROTOTYPE_TCP
	/*
	* 函数名称： parse_buffer
	* 使用说明：1、框架内会循环调用此函数直到函数返回0或者<0，每次调用最多只能读取一个包长。
	2、框架在收到一个完整的数据包时会将数据包投递至框架内的消息队列。
	* 输入参数： <const x_char_t* buf> 服务器接收到来自客户端的数据。
	* 输入参数： <const x_size_t len> 服务器接收到来自客户端的数据长度。
	* 输出参数：
	*	返回值 > 0 表示服务器已接收到一个完整数据包， 返回值表示数据包长度。
	*	返回值 = 0 表示服务器未接收到一个完整的数据包， 需要继续接收数据。
	*	返回值 < 0 表示服务器接收一个非法的数据包， 服务器会立即关闭链接、停止接收此客户端数据。
	*/
	x_int_t parse_buffer(const x_char_t* buf, const x_size_t len) 
	{ 
		return len; 
	}

	void on_io_accept(xworker_ptr peer_ptr)
	{
		
	}

	void on_io_connect(xconnector_ptr peer_ptr)
	{
		
	}

	void on_io_read(xworker_ptr peer_ptr, XRWBuffer &buffer)
	{
		
	}

	void on_io_write(xworker_ptr peer_ptr, XRWBuffer &buffer)
	{
		
	}

	void on_close(xworker_t *peer_ptr)
	{
		
	}

	void on_io_read(xconnector_ptr peer_ptr, XRWBuffer &buffer)
	{
		
	}

	void on_io_write(xconnector_ptr peer_ptr, XRWBuffer &buffer)
	{
		
	}

	void on_close(xconnector_t *peer_ptr)
	{
		
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP
	void on_io_accept(http_ptr peer_ptr)
	{
		
	}

	void on_io_accept(https_ptr peer_ptr)
	{
		
	}

	void on_io_upgrade(ws_ptr peer_ptr)
	{
		
	}

	void on_io_upgrade(wss_ptr peer_ptr)
	{
		
	}
#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_WEBSOCKET
	void on_io_accept(ws_ptr peer_ptr)
	{
		
	}

	void on_io_connect(ws_clt_ptr peer_ptr)
	{
		
	}
#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	void on_io_accept(wss_ptr peer_ptr)
	{
		
	}

	void on_io_connect(wss_clt_ptr peer_ptr)
	{
		
	}

#endif //

#if XSERVER_PROTOTYPE_HTTP
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
	}
	template <class Body, class Allocator,
			  class Send>
	void
	on_io_read(https_ptr peer_ptr,
			   boost::beast::string_view doc_root,
			   boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>> &&req,
			   Send &&send)
	{
	}

	void on_close(http_t *peer_ptr)
	{
		
	}

	void on_close(https_t *peer_ptr)
	{
		
	}

#endif //

#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_WEBSOCKET
	void on_io_read(ws_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_io_write(ws_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_close(ws_t *peer_ptr)
	{
		
	}

	void on_io_read(ws_clt_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_io_write(ws_clt_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_close(ws_clt_t *peer_ptr)
	{
		
	}

#endif //
#if XSERVER_PROTOTYPE_HTTP || XSERVER_PROTOTYPE_SSL_WEBSOCKET

	void on_io_read(wss_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_io_write(wss_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_close(wss_t *peer_ptr)
	{
		
	}

	void on_io_read(wss_clt_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_io_write(wss_clt_ptr peer_ptr, const std::string &buffer)
	{
		
	}

	void on_close(wss_clt_t *peer_ptr)
	{
		
	}

#endif //

protected:
	boost::unordered_map<size_t, boost::any> peer_map_;
	boost::shared_mutex peer_mutex_;
};

}

#endif //__H_XROUTER_H__
