#pragma once

#include "noncopyable.h"
#include "Thread.h"

#include <mutex>
#include <condition_variable>
#include <functional>
#include <string>
/*
 *用来绑定线程和其内部的loop
 *one loop per thread
 */
class EventLoop;
class EventLoopThread : noncopyable
{
public:
  using ThreadInitCallBack = std::function<void(EventLoop *)>;

  EventLoopThread(const ThreadInitCallBack &cb = ThreadInitCallBack(), const std::string &name = std::string());
  ~EventLoopThread();

  EventLoop *startLoop();

private:
  void threadFunc();

  EventLoop *loop_;
  bool existing_;
  Thread thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallBack callback_;
};
