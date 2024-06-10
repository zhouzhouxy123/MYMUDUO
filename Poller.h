#pragma once

#include "noncopyable.h"
#include "Timestamp.h"

#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;
/*
 *Poller本身是一个不能实例化的抽象类
 *muduo库对外赋予的IO复用能力有两个 poll和epoll
 *eventloop在使用IO复用的时候并不知道使用哪个，
 *而是在抽象的层面使用抽象类poller，使用不用的派生类对象调用同名复用方法
 *从而更方便的拓展不同IO复用的能力
 */

// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable
{
public:
  using ChannelList = std::vector<Channel *>;

  Poller(EventLoop *loop);
  virtual ~Poller() = default;

  // 给所有IO复用保留统一的接口
  // 在抽象类保留了统一的接口通过派生类不同自定义的实现，来实现不同的IO复用机制
  virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
  virtual void updateChannel(Channel *channel) = 0;
  virtual void removeChannel(Channel *channel) = 0;

  // 判断参数channel是否在当前Poller中
  bool hasChannel(Channel *channel) const;

  // EventLoop可以通过该接口获取默认的IO复用的具体实现
  // 没有把它放在poller.cc因为如果返回具体实现所使用的poll/epoll,需要引用派生类的文件
  // 但是在基类中引用派生类是一个不好的操作，所以作者专门写了一个DefaultPoller.cc来实现这个函数
  static Poller *newDefaultPoller(EventLoop *loop);

protected:
  // map的key：sockfd  value：sockfd所属的channel通道类型
  using ChannelMap = std::unordered_map<int, Channel *>;
  ChannelMap channels_;

private:
  // 定义Poller所属的事件循环EventLoop
  EventLoop *ownerLoop_;
};
