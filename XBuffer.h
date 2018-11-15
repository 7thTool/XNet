#ifndef __H_XBUFFER_H__
#define __H_XBUFFER_H__

#include "XType.h"

namespace XNet
{

template <class Server>
class XBuffer : private boost::noncopyable
{
  public:
	XBuffer() = default;
	~XBuffer() = default;

	void clear()
	{
		read_buffer_.clear();
		write_buffer_.clear();
	}

	bool getRead(std::string &buffer, Server *parse)
	{
		buffer.clear();
		if (parse)
		{
			size_t len = parse->parse_io_read(read_buffer_.c_str(), read_buffer_.size());
			if (len > 0)
			{
				if (len < read_buffer_.size())
				{
					buffer.assign(read_buffer_.begin(), read_buffer_.begin() + len);
					read_buffer_.erase(read_buffer_.begin(), read_buffer_.begin() + len);
				}
				else
				{
					buffer.swap(read_buffer_);
					return true;
				}
				return true;
			}
		}
		else
		{
			buffer.swap(read_buffer_);
			return true;
		}
		return false;
	}

	void appendRead(const char *buf, size_t len)
	{
		read_buffer_.append(buf, len);
	}

	std::deque<std::string> &getWrite(std::deque<std::string> &buffer)
	{
		buffer.clear();
		buffer.swap(write_buffer_);
		return buffer;
	}

	void appendWrite(const char *buf, size_t len)
	{
		write_buffer_.emplace_back(buf, len);
	}

	void prependWrite(const char *buf, size_t len)
	{
		write_buffer_.emplace_front(buf, len);
	}

  protected:
	std::string read_buffer_;
	std::deque<std::string> write_buffer_;
};

typedef union {
	x_int64_t n64;
	struct
	{
		x_int32_t n32_h;
		x_int32_t n32_l;
	};
} x_cov64_t;

/*
* 类名：x_r_buffer
* 说明：通讯内存只读处理类
*/

class x_r_buffer
{
  public:
	explicit x_r_buffer(const x_char_t *buf, x_size_t len, bool ne = false)
		: buffer_(buf), size_(len), ne_(ne), readerIndex_(0)
	{
		BOOST_ASSERT(readable() == size_);
	}

	void swap(x_r_buffer &rhs)
	{
		std::swap(buffer_, rhs.buffer_);
		std::swap(size_, rhs.size_);
		std::swap(readerIndex_, rhs.readerIndex_);
	}

	void reset()
	{
		readerIndex_ = 0;
	}

	x_size_t size() const
	{
		return size_ - readerIndex_;
	}

	x_size_t readable() const
	{
		return size_ - readerIndex_;
	}

	const x_char_t *data() const
	{
		return begin() + readerIndex_;
	}

	const x_char_t *reader() const
	{
		return begin() + readerIndex_;
	}

	void retrieve(x_size_t len)
	{
		BOOST_ASSERT(len <= readable());
		if (len < readable())
		{
			readerIndex_ += len;
		}
		else
		{
			reset();
		}
	}

	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}

	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}

	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}

	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}

	x_char_t *read(x_char_t *buf, x_size_t len)
	{
		peek(buf, len);
		retrieve(len);
		return buf;
	}

	template <class Y>
	Y &read(Y &rhs)
	{
		read(&rhs, sizeof(Y));
		return rhs;
	}

	x_int64_t readInt64(bool ne)
	{
		x_int64_t result = peekInt64(ne);
		retrieveInt64();
		return result;
	}

	x_int32_t readInt32(bool ne)
	{
		x_int32_t result = peekInt32(ne);
		retrieveInt32();
		return result;
	}

	x_int16_t readInt16(bool ne)
	{
		x_int16_t result = peekInt16(ne);
		retrieveInt16();
		return result;
	}

	x_int8_t readInt8(bool ne)
	{
		x_int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}

	x_char_t *peek(x_char_t *buf, x_size_t len)
	{
		BOOST_ASSERT(readable() >= len);
		::memcpy(buf, reader(), len);
		return buf;
	}
	template <class Y>
	Y &peek(Y &rhs)
	{
		peek(&rhs, sizeof(Y));
		return rhs;
	}
	x_int64_t peekInt64(bool ne) const
	{
		BOOST_ASSERT(readable() >= sizeof(x_int64_t));
		if (ne_ && !ne)
		{
			x_cov64_t x;
			::memcpy(&x.n64, reader(), sizeof(x_int64_t));
			x.n32_h = boost::asio::detail::socket_ops::network_to_host_long(x.n32_h);
			x.n32_l = boost::asio::detail::socket_ops::network_to_host_long(x.n32_l);
			return x.n64;
		}
		else if (!ne_ && ne)
		{
			x_cov64_t x;
			::memcpy(&x.n64, reader(), sizeof(x_int64_t));
			x.n32_h = boost::asio::detail::socket_ops::host_to_network_long(x.n32_h);
			x.n32_l = boost::asio::detail::socket_ops::host_to_network_long(x.n32_l);
			return x.n64;
		}
		x_int64_t x = 0;
		::memcpy(&x, reader(), sizeof(x_int64_t));
		return x;
	}

	x_int32_t peekInt32(bool ne) const
	{
		BOOST_ASSERT(readable() >= sizeof(x_int32_t));
		x_int32_t x = 0;
		::memcpy(&x, reader(), sizeof(x_int32_t));
		if (ne_ && !ne)
		{
			x = boost::asio::detail::socket_ops::network_to_host_long(x);
		}
		else if (!ne_ && ne)
		{
			x = boost::asio::detail::socket_ops::host_to_network_long(x);
		}
		return x;
	}

	x_int16_t peekInt16(bool ne) const
	{
		BOOST_ASSERT(readable() >= sizeof(x_int16_t));
		x_int16_t x = 0;
		::memcpy(&x, reader(), sizeof(x_int16_t));
		if (ne_ && !ne)
		{
			x = boost::asio::detail::socket_ops::network_to_host_short(x);
		}
		else if (!ne_ && ne)
		{
			x = boost::asio::detail::socket_ops::host_to_network_short(x);
		}
		return x;
	}

	x_int8_t peekInt8() const
	{
		BOOST_ASSERT(readable() >= sizeof(x_int8_t));
		x_int8_t x = *reader();
		return x;
	}

  protected:
	const x_char_t *begin() const
	{
		return buffer_;
	}

  protected:
	const x_char_t *buffer_;
	x_size_t size_;
	bool ne_;
	x_size_t readerIndex_;
};

/// x initial reserve (size) bytes read write buffer class
///
/// @code
/// +-------------------+------------------+------------------+
/// |  available bytes  |  readable bytes  |  writable bytes  |
/// |    available1     |     (CONTENT)    |    available2    |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
///
class XRWBuffer : private boost::noncopyable
{
  public:
	explicit XRWBuffer(size_t size = 1024)
		: buffer_(), readerIndex_(0), writerIndex_(0)
	{
		buffer_.reserve(size);
		BOOST_ASSERT(readable() == 0);
		BOOST_ASSERT(writable() == 0);
	}

	void swap(XRWBuffer &rhs)
	{
		buffer_.swap(rhs.buffer_);
		std::swap(readerIndex_, rhs.readerIndex_);
		std::swap(writerIndex_, rhs.writerIndex_);
	}

	void clear()
	{
		readerIndex_ = 0;
		writerIndex_ = 0;
	}

	size_t size() const
	{
		return writerIndex_ - readerIndex_;
	}

	size_t readable() const
	{
		return writerIndex_ - readerIndex_;
	}

	size_t writable() const
	{
		return buffer_.size() - writerIndex_;
	}

	size_t prependable() const
	{
		return readerIndex_;
	}

	size_t available() const
	{
		BOOST_ASSERT(writerIndex_ >= readerIndex_);
		return buffer_.size() - (writerIndex_ - readerIndex_);
	}

	size_t capacity() const
	{
		return buffer_.capacity();
	}

	const char *data() const
	{
		return begin() + readerIndex_;
	}

	const char *reader() const
	{
		return begin() + readerIndex_;
	}

	char *writer()
	{
		return begin() + writerIndex_;
	}

	const char *writer() const
	{
		return begin() + writerIndex_;
	}

	void retrieve(size_t len)
	{
		BOOST_ASSERT(len <= readable());
		if (len < readable())
		{
			readerIndex_ += len;
		}
		else
		{
			clear();
		}
	}

	void retrieveInt64()
	{
		retrieve(sizeof(int64_t));
	}

	void retrieveInt32()
	{
		retrieve(sizeof(int32_t));
	}

	void retrieveInt16()
	{
		retrieve(sizeof(int16_t));
	}

	void retrieveInt8()
	{
		retrieve(sizeof(int8_t));
	}

	void ensureWritable(size_t len)
	{
		if (writable() < len)
		{
			ensureWritableBytes(len);
		}
		BOOST_ASSERT(writable() >= len);
	}

	void write(size_t len)
	{
		BOOST_ASSERT(len <= writable());
		writerIndex_ += len;
	}

	void unwrite(size_t len)
	{
		BOOST_ASSERT(len <= readable());
		writerIndex_ -= len;
	}

	void append(const char *buf, size_t len)
	{
		ensureWritable(len);
		std::copy(buf, buf + len, writer());
		write(len);
	}

	void append(const void *buf, size_t len)
	{
		append(static_cast<const char *>(buf), len);
	}
	template <class Y>
	Y &append(const Y &rhs)
	{
		append(&rhs, sizeof(Y));
	}

	void appendVarint(int64_t x)
	{
		if (x < (int8_t)0xfd)
		{
			appendInt8((int8_t)x);
		}
		else if (x < (int16_t)0xffff)
		{
			appendInt8((int8_t)0xfd);
			appendInt16((int16_t)x);
		}
		else if (x < (int32_t)0xffffffffu)
		{
			appendInt8((int8_t)0xfe);
			appendInt32((int32_t)x);
		}
		else
		{
			appendInt8((int8_t)0xff);
			appendInt64((int64_t)x);
		}
	}

	void appendInt64(int64_t x)
	{
		append(&x, sizeof(int64_t));
	}

	void appendInt32(int32_t x)
	{
		append(&x, sizeof(int32_t));
	}

	void appendInt16(int16_t x)
	{
		append(&x, sizeof(int16_t));
	}

	void appendInt8(int8_t x)
	{
		append(&x, sizeof(x));
	}

	char *read(char *buf, size_t len)
	{
		peek(buf, len);
		retrieve(len);
		return buf;
	}

	template <class Y>
	Y &read(Y &rhs)
	{
		read(&rhs, sizeof(Y));
		return rhs;
	}

	int64_t readVarint()
	{
		int64_t result = 0;
		int8_t space = 0;
		int8_t mark = peekInt8();
		if (mark < (int8_t)0xfd)
		{
			result = mark;
			retrieveInt8();
			return result;
		}
		else if (mark == (int8_t)0xfd)
		{
			retrieveInt8();
			result = peekInt16();
			retrieveInt16();
			return result;
		}
		else if (mark == (int8_t)0xfe)
		{
			retrieveInt8();
			result = peekInt32();
			retrieveInt32();
			return result;
		}
		else
		{
			retrieveInt8();
			result = peekInt64();
			retrieveInt64();
			return result;
		}
	}

	int64_t readInt64()
	{
		int64_t result = peekInt64();
		retrieveInt64();
		return result;
	}

	int32_t readInt32()
	{
		int32_t result = peekInt32();
		retrieveInt32();
		return result;
	}

	int16_t readInt16()
	{
		int16_t result = peekInt16();
		retrieveInt16();
		return result;
	}

	int8_t readInt8()
	{
		int8_t result = peekInt8();
		retrieveInt8();
		return result;
	}

	char *peek(char *buf, size_t len)
	{
		BOOST_ASSERT(readable() >= len);
		::memcpy(buf, reader(), len);
		return buf;
	}
	template <class Y>
	Y &peek(Y &rhs)
	{
		peek(&rhs, sizeof(Y));
		return rhs;
	}

	int64_t peekInt64() const
	{
		BOOST_ASSERT(readable() >= sizeof(int64_t));
		int64_t x = 0;
		::memcpy(&x, reader(), sizeof(int64_t));
		return x;
	}

	int32_t peekInt32() const
	{
		BOOST_ASSERT(readable() >= sizeof(int32_t));
		int32_t x = 0;
		::memcpy(&x, reader(), sizeof(int32_t));
		return x;
	}

	int16_t peekInt16() const
	{
		BOOST_ASSERT(readable() >= sizeof(int16_t));
		int16_t x = 0;
		::memcpy(&x, reader(), sizeof(int16_t));
		return x;
	}

	int8_t peekInt8() const
	{
		BOOST_ASSERT(readable() >= sizeof(int8_t));
		int8_t x = *reader();
		return x;
	}

	void prepend(const char *buf, size_t len)
	{
		BOOST_ASSERT(len <= prependable());
		readerIndex_ -= len;
		std::copy(buf, buf + len, begin() + readerIndex_);
	}

	void prepend(const void *buf, size_t len)
	{
		prepend(static_cast<const char *>(buf), len);
	}
	template <class Y>
	Y &prepend(const Y &rhs)
	{
		prepend(&rhs, sizeof(Y));
	}

	void prependVarint(int64_t x)
	{
		if (x < (int8_t)0xfd)
		{
			prependInt8((int8_t)x);
		}
		else if (x < 0xffff)
		{
			prependInt8((int8_t)0xfd);
			prependInt16((int16_t)x);
		}
		else if (x < 0xffffffffu)
		{
			prependInt8((int8_t)0xfe);
			prependInt32((int32_t)x);
		}
		else
		{
			prependInt8((int8_t)0xff);
			prependInt64((int64_t)x);
		}
	}

	void prependInt64(int64_t x)
	{
		prepend(&x, sizeof(int64_t));
	}

	void prependInt32(int32_t x)
	{
		prepend(&x, sizeof(int32_t));
	}

	void prependInt16(int16_t x)
	{
		prepend(&x, sizeof(int16_t));
	}

	void prependInt8(int8_t x)
	{
		prepend(&x, sizeof(x));
	}

	void shrink()
	{
		buffer_.shrink_to_fit();
	}

  protected:
	char *begin()
	{
		return &*buffer_.begin();
	}

	const char *begin() const
	{
		return &*buffer_.begin();
	}

	void ensureWritableBytes(size_t len)
	{
		if (available() < len)
		{
			// FIXME: move readable data
			buffer_.resize(writerIndex_ + len);
		}
		else
		{
			// move readable data to the front, make space inside buffer
			BOOST_ASSERT(0 < readerIndex_);
			size_t readableSize = readable();
			std::copy(begin() + readerIndex_, begin() + writerIndex_, begin());
			readerIndex_ = 0;
			writerIndex_ = readerIndex_ + readableSize;
			BOOST_ASSERT(readableSize == readable());
		}
	}

  protected:
	std::string buffer_;
	size_t readerIndex_;
	size_t writerIndex_;
};

} // namespace XNet

#endif //__H_XBUFFER_H__
