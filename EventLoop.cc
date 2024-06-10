#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"

#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个loop one loop per thread
// 当一个线程中创建了一个EventLoop之后，这个指针就会去指向它
// __thread 确保是thread_local 每个线程只有一个eventloop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller，IO复用接口的时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，通过notify唤醒subReactor处理新来的channel
int createEventFd()
{
  int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
  if (evfd < 0)
  {
    LOG_FATAL("eventfd error:%d\n", errno);
  }
  return evfd;
}

EventLoop::EventLoop()
    : looping_(false), quit_(false), threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), wakeupFd_(createEventFd()), wakeupChannel(new Channel(this, wakeupFd_))
{
  LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);

  // one loop per thread
  if (t_loopInThisThread)
  {
    LOG_FATAL("Another EventLoo %p exists in this thread%d\n", t_loopInThisThread, threadId_);
  }
  else
  {
    t_loopInThisThread = this;
  }

  // 前面只是设置了wakeupfd,但是不确定怎么放到poller上以及fd的监听的事件
  // wakeup的本质就是唤醒subReator去处理事件
  // 设置wakeup的事件类型以及发生事件的回调
  wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
  wakeupChannel->enableReading();
}

EventLoop::~EventLoop()
{
  wakeupChannel->disableAll(); // 对所有事件不感兴趣
  wakeupChannel->remove();
  ::close(wakeupFd_);
  t_loopInThisThread = nullptr;
}

// 开始事件循环
void EventLoop::loop()
{
  looping_ = true;
  quit_ = false;

  while (!quit_)
  {
    activeChannels_.clear();
    // 监听两种fd，一种是clientfd,一种是wakeupfd
    pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);

    for (Channel *channel : activeChannels_)
    {
      // poller监听哪些channel发生了事件，然后上报给EventLoop，通知channel处理相关事件
      channel->handleEvent(pollReturnTime_);
    }
    // 执行当前EventLoop事件循环需要处理回调
    /*
     *IO线程 mainLoop accept fd《=channel subloop
     * mainLoop事先注册一个回调cb（需要通过subloop来执行）      wakeup subloop之后，执行下面的方法，注册之前mainloop注册的cb操作
     */
    // 实现把回调写在vector<Functor> pendingFunctors_中，subReactor唤醒之后去vector中寻找相应的回调去执行
    doPendingFunctors();
  }
  LOG_INFO("EventLoop %p stop looping\n", this);
  looping_ = false;
}

void EventLoop::quit()
{
  quit_ = true;
  // 如果在其他线程中，调用quit
  if (!isInLoopThread())
  {
    wakeup();
  }
}
// 在当前loop中执行回调
void EventLoop::runInLoop(Functor cb)
{
  if (isInLoopThread())
  {
    cb();
  }
  else
  {
    queueInLoop(cb);
  }
}
void EventLoop::queueInLoop(Functor cb)
{
  {
    // 并发访问
    std::unique_lock<std::mutex> lock(mutex_);
    pengdingFunctors.emplace_back(cb);
  }
  if (!isInLoopThread() || callingPendingFucntors_)
  {
    wakeup();
  }
}

void EventLoop::handleRead()
{
  uint64_t one = 1;
  ssize_t n = read(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleRead reads %lu bytes instead of 8", n);
  }
}
void EventLoop::wakeup()
{
  uint64_t one = 1;
  ssize_t n = write(wakeupFd_, &one, sizeof one);
  if (n != sizeof one)
  {
    LOG_ERROR("EventLoop::handleWrite writes %lu bytes instead of 8", n);
  }
}

void EventLoop::updateChannel(Channel *channel)
{
  poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel)
{
  poller_->removeChannel(channel);
}
void EventLoop::hasChannel(Channel *channel)
{
  poller_->hasChannel(channel);
}

void EventLoop::doPendingFunctors()
{
  std::vector<Functor> functors;
  callingPendingFucntors_ = true;

  {
    std::unique_lock<std::mutex> lock(mutex_);
    functors.swap(pengdingFunctors);
  }
  for (const Functor &functor : functors)
  {
    functor();
  }

  callingPendingFucntors_ = false;
}
