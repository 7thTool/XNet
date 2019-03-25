#ifndef __H_XNET_XNET_UTIL_HPP__
#define __H_XNET_XNET_UTIL_HPP__

#pragma once

#include "XType.hpp"
#include <XUtil/XStr.hpp>
#include <XUtil/XBuffer.hpp>
#include <XUtil/XLogger.hpp>

namespace XNet {
	// template <typename... Args>
	// inline auto LOG4D(Args... args) -> decltype(XUtil::LOG4D(std::forward<Args>(args)...)) {
  	// 	return XUtil::LOG4D(std::forward<Args>(args)...);
	// }
	// template <typename... Args>
	// inline auto LOG4I(Args... args) -> decltype(XUtil::LOG4I(std::forward<Args>(args)...)) {
  	// 	return XUtil::LOG4I(std::forward<Args>(args)...);
	// }
	// template <typename... Args>
	// inline auto LOG4W(Args... args) -> decltype(XUtil::LOG4W(std::forward<Args>(args)...)) {
  	// 	return XUtil::LOG4W(std::forward<Args>(args)...);
	// }
	// template <typename... Args>
	// inline auto LOG4E(Args... args) -> decltype(XUtil::LOG4E(std::forward<Args>(args)...)) {
  	// 	return XUtil::LOG4E(std::forward<Args>(args)...);
	// }
	//void (*g)() = &bar::f;
	//void (&h)() = bar::f;

	using XBuffer = XUtil::XBuffer;

	inline void fail(boost::system::error_code ec, char const* what)
	{
		std::cerr << what << ": " << ec.message() << "\n";
	}

	//template<typename Target>
	//using strto = Target(*)(const std::string&, const Target&);
}

#endif//__H_XNET_XNET_UTIL_HPP__


