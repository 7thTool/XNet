#ifndef __H_XUTIL_H_
#define __H_XUTIL_H_

#pragma once

#include "XType.h"

namespace XNet {

inline void fail(boost::system::error_code ec, char const* what)
{
	std::cerr << what << ": " << ec.message() << "\n";
}

template<typename Target>
Target strto(const std::string& arg, const Target& def = Target())
{
	if (!arg.empty()) {
		try
		{
			Target o;
			std::stringstream ss;
			ss << arg;
			ss >> o;
			return o;
		}
		catch (std::exception& e)
		{

		}
		catch (...)
		{

		}
	}
	return def;
}

template<typename Target>
Target wcsto(const std::wstring& arg, const Target& def = Target())
{
	if (!arg.empty()) {
		try
		{
			Target o;
			std::wstringstream ss;
			ss << arg;
			ss >> o;
			return o;
		}
		catch (std::exception& e)
		{

		}
		catch (...)
		{

		}
	}
	return def;
}

template<typename Source>
std::string tostr(const Source& arg)
{
	std::ostringstream ss;
	ss << arg;
	return ss.str();
}

template<typename Source>
std::wstring towcs(const Source& arg)
{
	std::wostringstream ss;
	ss << arg;
	return ss.str();
}

template<typename Source>
std::string tostrex(const Source& arg, int p = -1, int w = -1, char c = '0')
{
	std::ostringstream ss;
	if (p >= 0) {
		ss.setf(std::ios::fixed);
		ss.precision(p);
	}
	if (w >= 0) {
		ss.width(w);
		ss.fill(c);
	}
	ss << arg;
	return ss.str();
}

template<typename Source>
std::wstring towcsex(const Source& arg, int p = -1, int w = -1, wchar_t c = '0')
{
	std::wostringstream ss;
	if (p >= 0) {
		ss.setf(std::ios::fixed);
		ss.precision(p);
	}
	if (w >= 0) {
		ss.width(w);
		ss.fill(c);
	}
	ss << arg;
	return ss.str();
}

}

#endif//__H_XUTIL_H_


