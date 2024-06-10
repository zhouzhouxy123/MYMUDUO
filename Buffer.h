#pragma once

#include <vector>
#include <string>
#include <algorithm>

class Buffer
{
public:
  static const size_t kCheapPrepend = 8;   // 记录数据报的长度
  static const size_t kInitialSize = 1024; // 记录缓冲区大小

  explicit Buffer(size_t initialSize = kInitialSize)
      : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend){};

  size_t readabaleBytes() const
  {
    return writerIndex_ - readerIndex_;
  }

  size_t writeableBytes() const
  {
    return buffer_.size() - writerIndex_;
  }

  size_t prependableBytes() const
  {
    return readerIndex_;
  }

  // 返回缓冲区可读数据的起始位置
  const char *peek() const
  {
    return begin() + readerIndex_;
  }

  void retrieve(size_t len)
  {
    if (len < readabaleBytes())
    {
      readerIndex_ += len;
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveAll()
  {
    readerIndex_ = writerIndex_ = kCheapPrepend; // 表示当前没有可读数据
  }

  std::string retrieveAllAsString()
  {
    return retrieveAsString(readabaleBytes());
  }

  std::string retrieveAsString(size_t len)
  {
    std::string result(peek(), len); // 读取readable bytes部分
    retrieve(len);                   // 读取出来之后进行复位
    return result;
  }

  void ensureWriteableBytes(size_t len)
  {
    if (len < writeableBytes())
    {
      makeSpace(len); // 扩容函数
    }
  }

  // 把[data, data+len]内存上的数据，添加到writable缓冲区当中
  void append(const char *data, size_t len)
  {
    ensureWriteableBytes(len);
    std::copy(data, data + len, beginWrite());
    writerIndex_ += len;
  }
  char *beginWrite()
  {
    return begin() + writerIndex_;
  }
  const char *beginWrite() const
  {
    return begin() + writerIndex_;
  }

  // 从fd读数据
  size_t readFd(int fd, int *saveErro);
  // 通过fd发送数据
  size_t writeFd(int fd, int *saveErro);

private:
  char *begin()
  {
    return &*buffer_.begin();
  }
  const char *begin() const
  {
    return &*buffer_.begin();
  }

  void makeSpace(size_t len)
  {
    /*
        kCheapPrepend  |   reader   |   writer |
        kCheapPrepend  |           len            |
        */
    if (readabaleBytes() + prependableBytes() < len + kCheapPrepend)
    {
      buffer_.resize(writerIndex_ + len);
    }
    else
    {
      size_t readable = readabaleBytes(); // 未读数据
      std::copy(
          begin() + readerIndex_,
          begin() + writerIndex_,
          begin() + kCheapPrepend);
      readerIndex_ = kCheapPrepend;
      writerIndex_ = readerIndex_ + readable;
    }
  }

  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;
};
