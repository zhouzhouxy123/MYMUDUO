#include "Poller.h"
#include "EPollPoller.h"

#include <stdlib.h>

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
  if (::getenv("MUDUO_USE_POLL")) // 如果环境变量有这个实例
  {
    return nullptr; // 生成poll实例
  }
  else
  {
    return new EPollPoller(loop); // 生成epoll实例
  }
}
