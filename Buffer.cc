#include "Buffer.h"

#include <errno.h>
#include <sys/uio.h>
#include <unistd.h>
/*
 *从fd中读数据采用LT模式,如果有数据没有读完会发导致epoll_wait返回
 *Buffer缓冲区有大小，但是从fd中读数据，不知道tcp数据的大小=>写进两个缓冲区
 */
size_t Buffer::readFd(int fd, int *saveErro)
{
  char extrabuf[65536] = {0};

  struct iovec vec[2];

  const size_t writeable = writeableBytes();

  vec[0].iov_base = begin() + writerIndex_;
  vec[0].iov_len = writeable;

  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;

  const int iovcnt = (writeable < sizeof extrabuf) ? 2 : 1;
  // 使用两个缓冲区来读取数据
  const ssize_t n = ::readv(fd, vec, iovcnt);

  if (n < 0)
  {
    *saveErro = errno;
  }
  else if (n <= writeable)
  {
    writerIndex_ += n;
  }
  else
  {
    writerIndex_ = buffer_.size();
    append(extrabuf, n - writeable);
  }

  return n;
}

size_t Buffer::writeFd(int fd, int *saveErro)
{
  ssize_t n = ::write(fd, peek(), readabaleBytes());

  if (n < 0)
  {
    *saveErro = errno;
  }
  return n;
}
