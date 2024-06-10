#pragma once

#include <iostream>
#include <string>

// 时间类
class Timestamp
{
public:
  Timestamp();
  explicit Timestamp(const int64_t microSecondsSinceEpoch); // 带参数的构造函数都加了这一部分 防止隐式对象转换，加上explicit
  static Timestamp now();                                   // 静态方法，在没有对应类的时候也使用
  std::string toString() const;

private:
  int64_t microSecondsSinceEpoch_;
};
