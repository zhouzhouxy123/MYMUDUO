#pragma once

#include "noncopyable.h"
#include "Socket.h"
#include "Channel.h"

class EventLoop;
class InetAddress;
class Acceptor : noncopyable
{
public:
  using NewConnectionCallback = std::function<void(int sockfd, const InetAddress &)>;
  Acceptor(EventLoop *loop, const InetAddress &listenaddr, bool reuseport);
  ~Acceptor();

  void setConnectionCallBack(const NewConnectionCallback &cb)
  {
    newConnectionCallback_ = cb;
  }

  bool listening() { return listening_; }
  void listen();

private:
  void handleRead();

  EventLoop *loop_; // Acceptor使用的是用户定义的baseloop
  Socket acceptSocket_;
  Channel acceptChannel_;
  NewConnectionCallback newConnectionCallback_;
  bool listening_;
};
