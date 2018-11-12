#ifndef __H_XTYPE_H__
#define __H_XTYPE_H__

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

#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>

#ifndef _WIN32
#define nullptr 0
#endif

#include "XExport.h"

#define XSERVER_PROTOTYPE_BEAST (XSERVER_PROTOTYPE==XSERVER_ALL || XSERVER_PROTOTYPE==XSERVER_HTTP || XSERVER_PROTOTYPE==XSERVER_HTTPS || XSERVER_PROTOTYPE==XSERVER_WEBSOCKET || XSERVER_PROTOTYPE==XSERVER_SSL_WEBSOCKET)

#define PEER_TYPE_MASK			0xff
#define PEER_TYPE_BITS			8
#define PEER_ID_BITS 			(sizeof(size_t) * 8 - PEER_TYPE_BITS)
#define PEER_ID_MASK 			((size_t)~(PEER_TYPE_MASK << PEER_ID_BITS))
#define PEER_TYPE(id) 			((size_t)(((size_t)(id) ^ PEER_ID_MASK) >> PEER_ID_BITS))
#define PEER_ID(id)				((size_t)((size_t)(id) & PEER_ID_MASK))
#define MAKE_PEER_ID(type,id) 	((size_t)(((size_t)(type) << PEER_ID_BITS) | PEER_ID(id)))

#endif //__H_X_TYPE_H__
