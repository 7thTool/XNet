#ifndef __H_XNET_XIOSERVICE_HPP__
#define __H_XNET_XIOSERVICE_HPP__

#include "XType.hpp"

namespace XNet {

template <class Server>
class XIOService
{
  public:
	XIOService(Server &srv)
		: server_(srv)
	{
	}

	~XIOService()
	{
	}

	bool Start(int io_thread)
	{
		size_t i;

		IOService_.resize(io_thread);
		IOWork_.resize(io_thread);
		IOThread_.resize(io_thread);

		for (i = 0; i < IOThread_.size(); ++i)
		{
			IOService_[i] = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
			IOWork_[i] = boost::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(*IOService_[i]));
			IOThread_[i] = boost::shared_ptr<boost::thread>(new boost::thread(
				//boost::bind(&boost::asio::io_service::run, IOService_[i])
				[this, i]() {
					server_.on_io_init(i);
					IOService_[i]->run();
					server_.on_io_term(i);
				}));
		}

		return true;
	}

	void Stop()
	{
		size_t i;

		for (i = 0; i < IOThread_.size(); ++i)
		{
			IOService_[i]->stop();
			IOThread_[i]->join();
			IOService_[i]->reset();
		}

		IOWork_.clear();
		IOService_.clear();
		IOThread_.clear();
	}

	template <typename F>
	void Post(size_t index, F f)
	{
		//未使用strand来post任务时候 执行f顺序是不确定的
		//而使用了strand后 显示是按照f顺序依次执行
		get_service(index).post(f);
	}

	boost::asio::deadline_timer *CreateTimer(size_t index)
	{
		boost::asio::deadline_timer *timer_ptr = new boost::asio::deadline_timer(get_service(index));
		BOOST_ASSERT(timer_ptr);
		return timer_ptr;
	}
	template <typename F>
	void PostTimer(boost::asio::deadline_timer *timer_ptr, size_t millis, F f)
	{
		BOOST_ASSERT(timer_ptr);
		timer_ptr->expires_from_now(boost::posix_time::milliseconds(millis));
		timer_ptr->async_wait(f);
	}
	void KillTimer(boost::asio::deadline_timer *timer_ptr)
	{
		BOOST_ASSERT(timer_ptr);
		delete timer_ptr;
	}

	boost::asio::io_service &get_service(size_t index)
	{
		index = index % IOService_.size();
		return *IOService_[index];
	}

  private:
	Server &server_;
	std::vector<boost::shared_ptr<boost::asio::io_service>> IOService_;
	std::vector<boost::shared_ptr<boost::asio::io_service::work>> IOWork_;
	std::vector<boost::shared_ptr<boost::thread>> IOThread_;
};

}

#endif //__H_XNET_XIOSERVICE_HPP__
