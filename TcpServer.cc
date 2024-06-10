#include "TcpServer.h"
#include "Logger.h"
#include "TcpConnection.h"

#include <strings.h>
#include <functional>

// mainloop不能传入空指针
static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
  if (loop == nullptr)
  {
    LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                     const InetAddress &listenAddr,
                     const std::string &nameArg,
                     Option option)
    : loop_(CheckLoopNotNull(loop)), ipPort_(listenAddr.toIpPort()), name_(nameArg), acceptor_(new Acceptor(loop, listenAddr, option == KReusePort)), threadPool_(new EventLoopThreadPool(loop, name_)), connectionCallback_(), messageCallback_(), nextConnId_(1), started_(0)
{
  acceptor_->setConnectionCallBack(std::bind(
      &TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
}
