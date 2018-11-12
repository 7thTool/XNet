#ifndef __H_XWORKSERVICE_H__
#define __H_XWORKSERVICE_H__

#include <set>
#include <string>
#include <vector>
#include <unordered_set>
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

template <class Server>
class XWorkService
{
  public:
	XWorkService(Server &srv)
		: server_(srv)
	{
	}

	~XWorkService()
	{
	}

	bool Start(int work_thread)
	{
		size_t i;

		WorkService_.resize(work_thread);
		Work_.resize(work_thread);
		Strand_.resize(work_thread);
		WorkThread_.resize(work_thread);

		for (i = 0; i < WorkService_.size(); i++)
		{
			WorkService_[i] = boost::shared_ptr<boost::asio::io_service>(new boost::asio::io_service());
			Work_[i] = boost::shared_ptr<boost::asio::io_service::work>(new boost::asio::io_service::work(*WorkService_[i]));
			Strand_[i] = boost::shared_ptr<boost::asio::io_service::strand>(new boost::asio::io_service::strand(*WorkService_[i]));
			WorkThread_[i] = boost::shared_ptr<boost::thread>(new boost::thread(
				//boost::bind(&boost::asio::io_service::run, WorkService_[i])
				[this, i]() {
					server_.on_work_init(i);
					WorkService_[i]->run();
					server_.on_work_term(i);
				}));
		}

		return true;
	}

	void Stop()
	{
		size_t i;

		for (i = 0; i < WorkService_.size(); ++i)
		{
			WorkService_[i]->stop();
			WorkThread_[i]->join();
			WorkService_[i]->reset();
		}

		Strand_.clear();
		Work_.clear();
		WorkService_.clear();
		WorkThread_.clear();
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
		index = index % WorkService_.size();
		return *WorkService_[index];
	}

  private:
	boost::asio::io_service::strand &get_strand(size_t index)
	{
		index = index % Strand_.size();
		return *Strand_[index];
	}

  private:
	Server &server_;
	std::vector<boost::shared_ptr<boost::asio::io_service>> WorkService_;
	//使用work，即使没有异步io的情况下，也能保证io_service继续工作
	std::vector<boost::shared_ptr<boost::asio::io_service::work>> Work_;
	//使用strands最显著的好处就是简化我们的代码，因为通过strand来维护handler不需要显式地同步线程。
	//strands保证同属于一个strand的两个handler不会同时执行(在两个线程同时执行)。
	//如果你只使用一个IO线程（在Boost里面只有一个线程调用io_service::run），那么你不需要做任何的同步，此时已经是隐式的strand。
	//但是如果你想提高性能，因此使用多个IO线程，那么你有两种选择，一种是在不同的handler进行显式的同步，另一种就是使用strand。
	//使用strand将任务排序,即使在多线程情况下，我们也希望任务能按照post的次序执行
	std::vector<boost::shared_ptr<boost::asio::io_service::strand>> Strand_;
	std::vector<boost::shared_ptr<boost::thread>> WorkThread_;
	//boost::thread_group WorkThreadGroup_;
};

#endif //__H_XWORKSERVICE_H__
