#ifndef __H_XNET_XTYPE_HPP__
#define __H_XNET_XTYPE_HPP__

/*
* 框架宏： XSERVER_PROTOTYPE
* 使用说明： 定义XServer服务框架原型类型
*/
#define XSERVER_TCP 1				//TCP
#define XSERVER_HTTP 2				//HTTP/WS
#define XSERVER_HTTPS 3				//HTTPS/WSS
#define XSERVER_WEBSOCKET 4			//WS
#define XSERVER_SSL_WEBSOCKET 5		//WSS
#define XSERVER_ALL 6				//ALL

#define XSERVER_PROTOTYPE_TCP (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_TCP)
#define XSERVER_PROTOTYPE_HTTP	(XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_HTTP)
#define XSERVER_PROTOTYPE_HTTPS	(XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_HTTPS)
#define XSERVER_PROTOTYPE_WEBSOCKET (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET)
#define XSERVER_PROTOTYPE_SSL_WEBSOCKET (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET)
#define XSERVER_PROTOTYPE_BEAST (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_HTTP || XSERVER_PROTOTYPE==XSERVER_HTTPS || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET)
#define XSERVER_PROTOTYPE_SSL (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_HTTPS || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET)
//#define XSERVER_PROTOTYPE 3

#if XSERVER_PROTOTYPE==XSERVER_ALL
#define XSERVER_DEFAULT XSERVER_WEBSOCKET
#else
#define XSERVER_DEFAULT XSERVER_PROTOTYPE
#endif

/*
* PEER宏： PEER_TYPE
* 使用说明： 定义XServer通信peer类型
*/
#define PEER_TYPE_TCP 1
#define PEER_TYPE_TCP_CLIENT 2
#define PEER_TYPE_HTTP 3
#define PEER_TYPE_HTTP_CLIENT 4
#define PEER_TYPE_HTTPS 5
#define PEER_TYPE_HTTPS_CLIENT 6
#define PEER_TYPE_WEBSOCKET 7
#define PEER_TYPE_WEBSOCKET_CLIENT 8
#define PEER_TYPE_SSL_WEBSOCKET 9
#define PEER_TYPE_SSL_WEBSOCKET_CLIENT 10

#define XSERVER_VERSION_MAJOR 1
#define XSERVER_VERSION_MINOR 1
#define XSERVER_VERSION "1.1.1"

//#define XSERVER_THREAD_POOL 1

/*#ifdef _WIN32 
//定义_WIN32_WINNT 0x0501时，需要使用兼容XP设置
//#define _WIN32_WINNT 0x0501 
#define _WIN32_WINNT 0x0601 
#endif // _WIN32*/

#pragma warning(disable: 4251)

#include <set>
#include <deque>
#include <queue>
#include <vector>
#include <string>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>

#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/thread/shared_mutex.hpp>
#include <boost/atomic.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/array.hpp>
#include <boost/function.hpp>
#include <boost/date_time.hpp>
#include <boost/any.hpp>

#if XSERVER_PROTOTYPE_SSL
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#endif
/*
#ifndef _WIN32
#define nullptr 0
#endif

typedef char				x_char_t;
typedef unsigned char		x_byte_t;
typedef bool				x_bool_t;
typedef short				x_short_t;
typedef unsigned short		x_ushort_t;
#ifdef WIN32
typedef __int8				x_int8_t;
typedef unsigned __int8		x_uint8_t;
typedef __int16				x_int16_t;
typedef unsigned __int16	x_uint16_t;
typedef __int32				x_int32_t;
typedef unsigned __int32	x_uint32_t;
typedef __int64				x_int64_t;
typedef unsigned __int64	x_uint64_t;
typedef ptrdiff_t			x_int_t;
#else
typedef int8_t				x_int8_t;
typedef uint8_t				x_uint8_t;
typedef int16_t				x_int16_t;
typedef uint16_t			x_uint16_t;
typedef int32_t				x_int32_t;
typedef uint32_t			x_uint32_t;
typedef int64_t				x_int64_t;
typedef uint64_t			x_uint64_t;
typedef ssize_t				x_int_t;
#endif//
typedef size_t	            x_size_t;
typedef float				x_float_t;
typedef double				x_double_t;
typedef void*				x_voidptr_t;
*/
#define PEER_TYPE_BITS			8
#define PEER_ID_BITS 			(sizeof(size_t) * 8 - PEER_TYPE_BITS)
#define PEER_TYPE_MASK			((size_t)0xff << PEER_ID_BITS)
#define PEER_ID_MASK 			(~PEER_TYPE_MASK)
#define PEER_TYPE(id) 			(((size_t)(id) & PEER_TYPE_MASK) >> PEER_ID_BITS)
#define PEER_ID(id)				((size_t)(id) & PEER_ID_MASK)
#define MAKE_PEER_ID(type,id) 	(((size_t)(type) << PEER_ID_BITS) | PEER_ID(id))

#endif //__H_XNET_X_TYPE_HPP__
