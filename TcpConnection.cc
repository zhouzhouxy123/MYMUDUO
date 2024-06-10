#include "TcpConnection.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "EventLoop.h"

#include <functional>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <string>

static EventLoop *CheckLoopNotNull(EventLoop *loop)
{
  if (loop == nullptr)
  {
    LOG_FATAL("%s:%s:%d TcpConnection Loop is null! \n", __FILE__, __FUNCTION__, __LINE__);
  }
  return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                             const std::string &nameArg,
                             int sockfd,
                             const InetAddress &localAddr,
                             const InetAddress &peerAddr)
    : loop_(CheckLoopNotNull(loop)),
      name_(nameArg),
      state_(kConnecting),
      reading_(true),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop_, sockfd)),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      highWaterMark_(64 * 1024 * 1024)
{
  // 给channel设置相应回调函数，poller给channel通知感兴趣发生，channel需要回调相应操作
  channel_->setReadCallback(
      std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
  channel_->setWriteCallback(
      std::bind(&TcpConnection::handleWrite, this));
  channel_->setCloseCallback(
      std::bind(&TcpConnection::handleClose, this));
  channel_->setErrorCallback(
      std::bind(&TcpConnection::handleError, this));

  //.c_str() 方法将 std::string 转换为 C 风格的字符串（即 const char*），以便格式化到日志消息中。
  LOG_INFO("TcpConnectio::ctor[%s] at fd=%d\n", name_.c_str(), channel_->fd());
  socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
  LOG_INFO("TcpConnection::dtor[%s] at fd=%d\n", name_.c_str(), socket_);
}

// 发送数据
void TcpConnection::send(const std::string &buf)
{
  if (state_ == kConnected)
  {
    // 如果是当前线程
    if (loop_->isInLoopThread())
    {
      sendInLoop(buf.c_str(), sizeof buf);
    }
    else
    {
      // 不是的话设置回调
      loop_->runInLoop(std::bind(
          &TcpConnection::sendInLoop,
          this,
          buf.c_str(),
          buf.size()));
    }
  }
}
/**
 * 发送数据  应用写的快， 而内核发送数据慢， 需要把待发送数据写入缓冲区， 而且设置了水位回调
 */
void TcpConnection::sendInLoop(const void *data, size_t len)
{
  ssize_t nwrote = 0;
  size_t remaining = len;
  bool faultError = false;

  if (state_ == kDisconnected)
  {
    LOG_ERROR("diconnected,give up writing!\n");
    return;
  }

  // 表示channel_第一次写数据
  if (!channel_->isWritng() && outputBuffer_.readabaleBytes() == 0)
  {
    nwrote = ::write(channel_->fd(), data, len);
    if (nwrote >= 0)
    {
      remaining = len - nwrote;
      if (remaining == 0 && writeCompleteCallback_)
      {
        loop_->queueInLoop(
            std::bind(writeCompleteCallback_, shared_from_this()));
      }
    }
    else
    {
      nwrote = 0;
      if (errno != EWOULDBLOCK)
      {
        LOG_ERROR("TcpConnection::sendInLoop");
        if (errno == EPIPE || errno == ECONNRESET) // SIGPIPE  RESET
        {
          faultError = true;
        }
      }
    }
  }
  // 说明当前这一次write，并没有把数据全部发送出去，剩余的数据需要保存到缓冲区当中，然后给channel
  // 注册epollout事件，poller发现tcp的发送缓冲区有空间，会通知相应的sock-channel，调用writeCallback_回调方法
  // 也就是调用TcpConnection::handleWrite方法，把发送缓冲区中的数据全部发送完成
  if (!faultError && remaining > 0)
  {
    // 剩余长度
    size_t oldLen = outputBuffer_.readabaleBytes();
    if (oldLen + remaining >= highWaterMark_ && oldLen < highWaterMark_ && highWaterMarkCallback_)
    {
      loop_->queueInLoop(
          std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
    }
    outputBuffer_.append((char *)data + nwrote, remaining); // 将数据追加到发送缓冲区
    if (!channel_->isWritng())
    {
      channel_->enableWriting();
    }
  }
}
// 关闭连接
void TcpConnection::shutdown()
{
  if (state_ == kConnected)
  {
    setState(kDisConnecting);
    loop_->runInLoop(
        std::bind(&TcpConnection::shutdownInLoop, this));
  }
}

void TcpConnection::shutdownInLoop()
{
  if (!channel_->isWritng()) // 说明outbuffer中数据全部发送出去
  {
    socket_->shutdownWrite();
  }
}

// 连接建立
void TcpConnection::connectEstablished()
{
  setState(kConnected);
  channel_->tie(shared_from_this());
  channel_->enableReading();

  connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
  if (state_ == kConnected)
  {
    setState(kDisconnected);
    channel_->disableAll();
    connectionCallback_(shared_from_this());
  }
  channel_->remove();
}

// fd上有数据可读
void TcpConnection::handleRead(Timestamp receiveTime)
{
  int saveError = 0;
  // 一定是可读
  size_t n = inputBuffer_.readFd(channel_->fd(), &saveError);
  if (n > 0)
  {
    // 已建立连接的用户，有可读事件发生，调用用户传入的回调操作
    // TODO:shared_from_this()获取当前tcpconnection对象的智能指针 (可以理解为this指针)
    messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
  }
  else if (n == 0)
  {
    handleClose();
  }
  else
  {
    // 客户端断开
    errno = saveError;
    LOG_ERROR("TcpConnection::handleRead");
    handleError();
  }
}
void TcpConnection::handleWrite()
{
  // channel写状态检查
  if (channel_->isWritng())
  {
    int saveErrno = 0;
    size_t n = outputBuffer_.writeFd(channel_->fd(), &saveErrno);
    // 写入成功处理
    if (n > 0)
    {
      outputBuffer_.retrieve(n);
      if (outputBuffer_.readabaleBytes() == 0)
      {
        // 数据写完了
        channel_->disableWriting();
        // 如果设置了写完成回调将其加入事件循环队列
        if (writeCompleteCallback_)
        {
          // 唤醒loop对应的thread线程
          loop_->queueInLoop(
              std::bind(writeCompleteCallback_, shared_from_this()));
        }
        // 在当前loop中删除TcpConnectionptr
        if (state_ == kDisConnecting)
        {
          shutdownInLoop();
        }
      }
    }
    else
    {
      LOG_ERROR("TcpConnection::handleWrite");
    }
  }
  else
  {
    LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
  }
}
// poller => channel::closeCallback => TcpConnection::handleClose
void TcpConnection::handleClose()
{
  LOG_INFO("TcpConnection::handleClose fd = %d  state = %d\n", channel_->fd(), (int)state_);
  setState(kDisconnected);
  channel_->disableAll();

  TcpConnectionPtr connPtr(shared_from_this());
  connectionCallback_(connPtr); // 执行连接关闭的回调
  closeCallback_(connPtr);      // 关闭连接的回调
}
void TcpConnection::handleError()
{
  int optval;
  socklen_t optlen = sizeof optval;
  int err = 0;
  if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen))
  {
    err = errno;
  }
  else
  {
    err = optval;
  }
  LOG_ERROR("TcpCpnnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}
