#include "Socket.h"
#include "Logger.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

Socket::~Socket()
{
  close(sockfd_);
}

// 对socket绑定ip和port
void Socket::bindAddress(const InetAddress &localAddr)
{
  if (0 != ::bind(sockfd_, (sockaddr *)&localAddr.getSockAddr(), sizeof localAddr))
  {
    LOG_FATAL("bind sockfd:%d fail\n", sockfd_);
  }
}
void Socket::listen()
{
  // ::表示调用的是全局的listen，避免和用户定义的listen函数发生冲突
  if (0 != ::listen(sockfd_, 1024))
  {
    LOG_FATAL("listen sockfd:%d fail\n", sockfd_);
  }
}
int Socket::accept(InetAddress *peeraddr)
{
  sockaddr_in addr;
  bzero(&addr, sizeof addr);
  socklen_t len;
  int connfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
  if (connfd >= 0)
  {
    peeraddr->setSockAddr(addr);
  }
  return connfd;
}

void Socket::shutdownWrite()
{
  if (::shutdown(sockfd_, SHUT_WR) < 0)
  {
    LOG_ERROR("shutdownWrite error");
  }
}

void Socket::setTcpNoDelay(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}
void Socket::setReuseAddr(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}
void Socket::setReusePort(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}
void Socket::setKeepAlive(bool on)
{
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}
