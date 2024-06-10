#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;
class EventLoopThreadPool : noncopyable
{
public:
  using ThreadInitCallBack = std::function<void(EventLoop *)>;

  EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
  ~EventLoopThreadPool();

  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  void start(const ThreadInitCallBack &cb = ThreadInitCallBack());

  EventLoop *getNextLoop();

  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }
  const std::string name() const { return name_; }

private:
  // 用户创建的loop，baseloop
  EventLoop *baseLoop_;
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_; // 创建所有EventLoop的线程
  std::vector<EventLoop *> loops_;                        // Eventloop对应的指针
};
