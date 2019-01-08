#ifndef __H_XNET_XNET_UTIL_HPP__
#define __H_XNET_XNET_UTIL_HPP__

#pragma once

#include "XType.hpp"
#include <XUtil/XStr.hpp>

namespace XNet {

inline void fail(boost::system::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

//template<typename Target>
//using strto = Target(*)(const std::string&, const Target&);

}

#endif//__H_XNET_XNET_UTIL_HPP__


