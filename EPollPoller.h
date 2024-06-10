#pragma once

#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

class Channel;
/*
 * epoll的使用和
 * epoll_create
 * epoll_ctl add/mod/del
 * eopll_wait
 */
class EPollPoller : public Poller
{
public:
  EPollPoller(EventLoop *loop);
  ~EPollPoller() override;

  // 重写基类Poller中的方法
  Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
  void updateChannel(Channel *channel) override;
  void removeChannel(Channel *channel) override;

private:
  // 给epoll_event的vector的初始长度
  static const int kInitEventListSize = 16;

  // 填写活跃的连接
  void fillActiveChannels(int numEvents, ChannelList *activeChannel) const;
  // 更新channel通道
  void update(int operation, Channel *channel);

  using EventList = std::vector<epoll_event>; // epoll_event可以扩容

  int epollfd_;
  EventList events_;
};
