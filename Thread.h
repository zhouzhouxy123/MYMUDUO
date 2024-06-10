#pragma once

#include <memory>
#include <thread>
#include <string>
#include <atomic>
#include <functional>

class Thread
{
public:
  using ThreadFunc = std::function<void()>;

  explicit Thread(ThreadFunc, const std::string &name = std::string()); // std::string()是默认值
  ~Thread();

  void start();
  void join();

  bool started() const { return started_; }
  pid_t tid() const { return tid_; }
  const std::string &name() const { return name_; } // threadName 是 const std::string&,threadName 不能被修改

  static int numCreated() { return numCreated_; }

private:
  void setDefaultname();

  bool started_;                        // 是否启动
  bool joined_;                         // 是否join
  std::shared_ptr<std::thread> thread_; // 封装在智能指针中，来控制线程对象启动的时机
  pid_t tid_;                           // 线程id
  ThreadFunc func_;                     // 线程函数
  std::string name_;                    // 线程名称
  static std::atomic_int numCreated_;   // 静态全局变量，用来记录线程个数
};
