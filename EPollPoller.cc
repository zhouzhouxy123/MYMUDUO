#include "EPollPoller.h"
#include "Logger.h"
#include "Channel.h"

#include <errno.h>
#include <unistd.h>
#include <strings.h>

// 用来判断channel是否添加到poller中
//  channel 未添加到channel中
const int kNew = -1;
// channel 已添加到channel中
const int kAdded = 1;
// channel从poller中删除
const int kDeleted = 2;

// ::epoll_create1(EPOLL_CLOEXEC)通过epoll create创建epollfd，当前线程fork创建子进程用exec替换子进程的程序时在子进程设置的fd都关闭
EPollPoller::EPollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) // vector<eopll_event>的默认长度，epoll_wait会把发生事件的event放在vector序列中
{
  if (epollfd_)
  {
    LOG_FATAL("epoll_create error:%d\n", errno);
  }
}

EPollPoller::~EPollPoller()
{
  ::close(epollfd_);
}

// poll的作用是epoll_wait监听哪些fd产生事件，然后通过activeChannels返回给EventLoop，告知事件发生
Timestamp EPollPoller::poll(int timeoutMs, ChannelList *activeChannels)
{
  LOG_INFO("func = %s=>fd total count = %lu\n", __FUNCTION__, channels_.size());

  int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
  int saveErrno = errno; // errno是全局变量
  Timestamp now(Timestamp::now());

  if (numEvents > 0)
  {
    LOG_INFO("%d events` happened\n", numEvents);
    fillActiveChannels(numEvents, activeChannels);
    if (numEvents == events_.size())
    {
      events_.resize(events_.size() * 2);
    }
  }
  else if (numEvents == 0) // 没有fd发生事件
  {
    LOG_DEBUG("%d events happened\n", __FUNCTION__);
  }
  else
  {
    // 无外部终端
    if (saveErrno != EINTR)
    {
      // 为了记录当前系统发生的errno的值，因为它是全局变量，在执行过程中可能有别的程序对它进行改变，所以我们将局部变量保存当前程序的值还给它
      errno = saveErrno;
      LOG_ERROR("EPollPoller::poll() err");
    }
  }
  return now;
}
// channel 在update，remove的时候无法自行操作需要=》EventLoop去UpdateChannel 和removeChannel，而EventLoop包含Channel和EPollPoller两部分，所以它需要EPollPoller去执行=》 EpollPOller 的updateChannel和removeChannel
/**
 *            EventLoop  =>   poller.poll
 *     ChannelList      Poller
 *                     ChannelMap  <fd, channel*>   epollfd
 *  EventLoop包含所有的channel
 *  Poller中表示如果记录过这个channel就把<fd, channel*>键值对记录在ChannelMap中
 */
void EPollPoller::updateChannel(Channel *channel)
{
  const int index = channel->index(); // 对应epoller的三种状态
  LOG_INFO("func= %s => fd = %d events = %d index = %d\n", __FUNCTION__, channel->fd(), channel->events(), channel->index());

  if (index == kNew || index == kDeleted)
  {
    if (index == kNew)
    {
      int fd = channel->fd();
      channels_[fd] = channel;
    }
    channel->set_index(kAdded);
    update(EPOLL_CTL_ADD, channel);
  }
  else
  {
    int fd = channel->fd();
    if (channel->isNoneEvent())
    {
      update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    }
    else
    {
      update(EPOLL_CTL_MOD, channel);
    }
  }
}
// 将epoll从epoller中删除
void EPollPoller::removeChannel(Channel *channel)
{
  int fd = channel->fd();
  channels_.erase(fd); // 将poller从channelmap中删除

  LOG_INFO("func = %s => fd =%d\n", __FUNCTION__, channel->fd());

  int index = channel->index();
  if (index == kAdded)
  {
    update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kDeleted);
}

void EPollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannel) const
{
  for (int i = 0; i < numEvents; i++)
  {
    Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
    channel->set_revents(events_[i].events);
    activeChannel->push_back(channel); // EventLoop就拿到了它的poller给它返回的所有发生事件的channel列表了
  }
}

void EPollPoller::update(int operation, Channel *channel)
{
  epoll_event event;
  bzero(&event, sizeof(event));

  int fd = channel->fd();

  event.events = channel->events();
  event.data.fd = fd;       // 我们要监听的fd
  event.data.ptr = channel; // fd对应的channel

  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
  {
    if (operation == EPOLL_CTL_DEL)
    {
      LOG_ERROR("epoll_ctl del eroor:%d\n", errno);
    }
    else
    {
      LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
    }
  }
}
