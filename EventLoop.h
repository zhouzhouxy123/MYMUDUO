#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类 主要包含了两大模块 Channel Poller(epoll的抽象)
class EventLoop
{
public:
  using Functor = std::function<void()>;

  EventLoop();
  ~EventLoop();

  // 开始事件循环
  void loop();
  // 退出事件循环
  void quit();

  Timestamp pollReturnTime() const { return pollReturnTime_; }

  // 在当前loop执行cb
  void runInLoop(Functor cb);
  // 把cb放入队列中，唤醒loop所在线程，执行cb
  void queueInLoop(Functor cb);

  // 用来唤醒loop所在的线程的
  void wakeup();

  // EventLoop方法=》channel方法
  void updateChannel(Channel *channel);
  void removeChannel(Channel *channel);
  void hasChannel(Channel *channel);

  // 判断EventLoop对象是否在自己的线程里
  bool isInLoopThread() { return threadId_ == CurrentThread::tid(); }

private:
  void handleRead();        // wake up
  void doPendingFunctors(); // 执行回调

  using ChannelList = std::vector<Channel *>;

  std::atomic_bool looping_; // 原子操作，通过CAS实现
  std::atomic_bool quit_;    // 标识退出loop循环操作

  const pid_t threadId_; // 记录loop当前线程id

  Timestamp pollReturnTime_;       // poll返回发生事件的channel的时间点
  std::unique_ptr<Poller> poller_; // 用智能指针动态管理资源

  int wakeupFd_; // 当mainLoop获取到新用户的channel,通过轮询算法选择一个subLoop，通过该成员唤醒subLoop处理channel
  std::unique_ptr<Channel> wakeupChannel;

  ChannelList activeChannels_;

  std::atomic_bool callingPendingFucntors_; // 表示当前loop是否有需要执行的回调操作

  std::vector<Functor> pengdingFunctors; // 存储loop需要执行的所有回调操作
  std::mutex mutex_;                     // 互斥锁，用来保护vector上面的线程安全
};
