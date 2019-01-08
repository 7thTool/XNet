#ifndef __H_XNET_XLOGGER_HPP__
#define __H_XNET_XLOGGER_HPP__

#pragma once

#include "XType.hpp"
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>

#ifdef WIN32
#ifndef vsnprintf
#define vsnprintf vsprintf_s
#endif
#endif

namespace XNet
{

// template<typename T0>
// void printf(T0 val) {
//     std::cout << val;
// }
// template<typename T, typename... Args>
// void printf(T val, Args... args) {
//     std::cout << val;
//     printf(args...);
// }
// template<typename T0, typename... T>
// void printf(T0 t0, T... t) {
//     std::cout << t0;
//     if constexpr (sizeof...(t) > 0) printf(t...);
// }

class XLogger : private boost::noncopyable
{
  public:
	static XLogger &instance()
	{
		static XLogger slogger;
		return slogger;
	}

	static void init(
		const std::string &filename,
		boost::log::trivial::severity_level lv = boost::log::trivial::debug,
		int maxsize = 100 * 1000 * 1000)
	{
		boost::log::register_simple_formatter_factory<boost::log::trivial::severity_level, char>("Severity");

		boost::log::add_file_log(
			filename + "_%Y-%m-%d-%N.log",
			boost::log::keywords::auto_flush = true,
			boost::log::keywords::open_mode = std::ios::app,
			boost::log::keywords::rotation_size = maxsize,
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
			boost::log::keywords::format = "%TimeStamp% %Severity% : %Message%");

		boost::log::core::get()->set_filter(boost::log::trivial::severity >= lv);
		boost::log::add_common_attributes();
	}

	void write(boost::log::trivial::severity_level lv, const char *fmt, ...)
	{
		char buffer[2048] = {0};
		va_list args;
		va_start(args, fmt);
		vsnprintf(buffer, sizeof(buffer) - 1, fmt, args);
		BOOST_LOG_SEV(logger_, lv) << buffer;
		va_end(args);
	}

	void write_direct(boost::log::trivial::severity_level lv, const char *str)
	{
		BOOST_LOG_SEV(logger_, lv) << str;
	}

  private:
	boost::log::sources::severity_logger<boost::log::trivial::severity_level> logger_;
};

//#define XNET_LOG4D(format, ...) \
//	XNet::XLogger::instance().write(boost::log::trivial::debug, format, ##__VA_ARGS__)
//#define XNET_LOG4I(format, ...) \
//	XNet::XLogger::instance().write(boost::log::trivial::info, format, ##__VA_ARGS__)
//#define XNET_LOG4W(format, ...) \
//	XNet::XLogger::instance().write(boost::log::trivial::warning, format, ##__VA_ARGS__)
//#define XNET_LOG4E(format, ...) \
//	XNet::XLogger::instance().write(boost::log::trivial::error, format, ##__VA_ARGS__)
//#define XNET_LOG4F(format, ...) \
//	XLogger::instance().write(boost::log::trivial::fatal, format, ## __VA_ARGS__)

template <typename... Args>
void LOG4D(const char *fmt, Args... args)
{
	XLogger::instance().write(boost::log::trivial::debug, fmt, args...);
}

template <typename... Args>
void LOG4I(const char *fmt, Args... args)
{
	XLogger::instance().write(boost::log::trivial::info, fmt, args...);
}

template <typename... Args>
void LOG4W(const char *fmt, Args... args)
{
	XLogger::instance().write(boost::log::trivial::warning, fmt, args...);
}

template <typename... Args>
void LOG4E(const char *fmt, Args... args)
{
	XLogger::instance().write(boost::log::trivial::error, fmt, args...);
}

} // namespace XNet

#endif //__H_XNET_XLOGGER_HPP__
