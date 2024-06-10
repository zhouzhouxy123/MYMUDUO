#pragma once

/*
 *TcpServer=>Acceptor(主线程监听)=> 有一个新用户连接通过accpet拿到connfd
 *=> TcpConnection设置回调=》 Channel => Poller => Channel 的回调操作
 */

#include "noncopyable.h"
#include "Logger.h"
#include "Callbacks.h"
#include "InetAddress.h"
#include "Buffer.h"
#include "Timestamp.h"

#include <string>
#include <memory>
#include <atomic>

class EventLoop;
class Socket;
class Channel;
class TcpConnection : noncopyable, public std::enable_shared_from_this<TcpConnection>
{
public:
  TcpConnection(EventLoop *loop,
                const std::string &namArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr);
  ~TcpConnection();

  EventLoop *getLoop() { return loop_; }
  const std::string &name() const { return name_; }
  const InetAddress &localAddress() const { return localAddr_; }
  const InetAddress &peerAddress() const { return peerAddr_; }

  bool conncted() { return state_ == kConnected; }

  // 发送数据
  void send(const std::string &buf);
  // 关闭连接
  void shutdown();

  void setConnectionCallback(const ConnectionCallback &cb)
  {
    connectionCallback_ = cb;
  }

  void setMessageCallback(const MessageCallback &cb)
  {
    messageCallback_ = cb;
  }

  void setWriteCompleteCallback(const WriteCompleteCallback &cb)
  {
    writeCompleteCallback_ = cb;
  }

  void setHighWaterMarkCallBack(const HighWaterMarkCallback &cb, size_t highWaterMark)
  {
    highWaterMarkCallback_ = cb;
    highWaterMark_ = highWaterMark;
  }
  void setCloseCallback(const CloseCallback &cb)
  {
    closeCallback_ = cb;
  }

  // 连接建立
  void connectEstablished();
  // 连接销毁
  void connectDestroyed();

private:
  // 表示连接状态
  enum StateE
  {
    kDisconnected,
    kConnecting,
    kConnected,
    kDisConnecting
  };
  void setState(StateE state) { state_ = state; }

  void handleRead(Timestamp receiveTime);
  void handleWrite();
  void handleClose();
  void handleError();

  void sendInLoop(const void *data, size_t len);
  void shutdownInLoop();

  EventLoop *loop_; // 这里不是mainloop,Tcpconntion是在subloop中管理
  std::string name_;
  std::atomic_int state_;
  bool reading_;

  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;

  const InetAddress localAddr_;
  const InetAddress peerAddr_;

  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  HighWaterMarkCallback highWaterMarkCallback_;
  CloseCallback closeCallback_;
  size_t highWaterMark_;

  Buffer inputBuffer_;  // 接收数据的缓冲区
  Buffer outputBuffer_; // 发送数据的缓冲区
};
