#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallBack &cb, const std::string &name)
    : loop_(nullptr), existing_(false), thread_(std::bind(&EventLoopThread::threadFunc, this), name), mutex_(), cond_(), callback_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
  existing_ = true;
  if (loop_ != nullptr)
  {
    loop_->quit();  // 退出线程绑定的事件循环
    thread_.join(); // 等待子线程结束
  }
}

// 获取新线程，在线程内单独运行了一个loop对象，把loop地址返回
EventLoop *EventLoopThread::startLoop()
{
  thread_.start(); // 启动底层的新线程

  EventLoop *loop = nullptr;
  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == nullptr)
    {
      cond_.wait(lock); // 等待loop_创建成功
    }
  }
  loop = loop_;
  return loop;
}

// TODO:多理解
//  下面这个方法，是在单独的线程中运行
//  因为每次start都单独开启一个线程 one loop per thread
void EventLoopThread::threadFunc()
{
  EventLoop loop; // 创建一个独立的eventloop，和上面的线程是一一对应的，one loop per thread

  if (callback_)
  {
    callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;      // 成员新创建的loop
    cond_.notify_one(); // 通知等待的线程EventLoop已经创建并初始化完毕
  }

  loop.loop(); // EventLoop loop  => Poller.poll
  // 当事件循环结束后，再次获取互斥锁 mutex_，将成员变量 loop_ 置为空。
  // 这表示 EventLoop 对象的生命周期结束。
  std::unique_lock<std::mutex> lock(mutex_);
  loop_ = nullptr;
}
