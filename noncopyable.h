#pragma once

/*
 *noncopayable被继承以后，派生类对象可以进行正常的构造和析构，但是派生类对象
 *无法进行拷贝构造和复制操作
 *也就是说，日后如果我们需要派生类无法进行拷贝构造和复制就可以使用这种方法
 */
class noncopyable
{
public:
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;

protected:
  noncopyable() = default;
  ~noncopyable() = default;
};
