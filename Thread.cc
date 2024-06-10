#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

std::atomic_int Thread::numCreated_(0); // 静态成员变量需要在类外定义

Thread::Thread(ThreadFunc func, const std::string &name)
    : started_(false), joined_(false), tid_(0), func_(std::move(func)), name_(name)
{
  setDefaultname();
}

Thread::~Thread()
{
  if (started_ && !joined_)
  {
    thread_->detach(); // 线程分离，Thread对象不再拥有这个线程
  }
}

void Thread::start()
{
  started_ = true;
  sem_t sem; // 设置信号量
  sem_init(&sem, false, 0);

  // 开启线程
  thread_ = std::shared_ptr<std::thread>(new std::thread([&]()
                                                         {
    //获取线程id
    tid_ = CurrentThread::tid();
    sem_post(&sem);
    //开启一个新线程专门执行该线程函数
    func_(); }));
  /*
   *如果当前没有资源，信号量sem会把start()函数阻塞住，在sem_post(&sem)，给信号量资源加一之后，说明子线程的tid值以及存在了，通过这种方法，当执行完start就可以放心的拿子线程的tid，此时一定以及创建完成
   */
  sem_wait(&sem);
}

void Thread::join()
{
  joined_ = true;
  thread_->join();
}

void Thread::setDefaultname()
{
  int num = ++numCreated_;
  if (name_.empty())
  {
    char buf[32] = {0};
    snprintf(buf, sizeof buf, "Thread%d", num);
    name_ = buf;
  }
}
