#pragma once

#include <unistd.h>
#include <sys/syscall.h>

// 这些函数和变量与特定对象无关，更像是一组工具函数和变量，适合放在命名空间中，而不是类中。
namespace CurrentThread
{
  //__thread线程局部，每个线程都有这个
  extern __thread int t_cachedTid;

  void cachedTid();

  inline int tid()
  {
    //__builtin_expect 是 GCC（GNU Compiler Collection）提供的一个内建函数，用于向编译器提供分支预测信息
    // long __builtin_expect(long exp, long c);
    // 期望t_cachedTid == 0为假
    if (__builtin_expect(t_cachedTid == 0, 0))
    {
      cachedTid();
    }
    return t_cachedTid;
  }
}
