#pragma once

#include <string>

#include "noncopyable.h"

// 我们在使用的时候希望的是直接调用LOG_INFO...，不需要自己再去创建类的对象
//  LOG_INFO("%s %d",arg1,arg2)
#define LOG_INFO(logmsgFormat, ...)                   \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::getinstance();           \
    logger.setLogLevel(INFO);                         \
    char buf[1024];                                   \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0)

#define LOG_ERROR(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::getinstance();           \
    logger.setLogLevel(ERROR);                        \
    char buf[1024];                                   \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
    exit(-1);                                         \
  } while (0)

#define LOG_FATAL(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::getinstance();           \
    logger.setLogLevel(FATAL);                        \
    char buf[1024];                                   \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0)

// debug会有很多调试信息占用很多IO资源，所以我们只有调用MUDEBUG的情况我们才输出LOG_DEBUG的信息
#ifdef MUDEBUG
#define LOG_DEBUG(logmsgFormat, ...)                  \
  do                                                  \
  {                                                   \
    Logger &logger = Logger::getinstance();           \
    logger.setLogLevel(DEBUG);                        \
    char buf[1024];                                   \
    snprintf(buf, 1024, logmsgFormat, ##__VA_ARGS__); \
    logger.log(buf);                                  \
  } while (0)
#else
#define LOG_DEBUG(LogmsgFormat, ...)
#endif

// 定义日志的级别  INFO ERRO FATAL DEBUG
enum LogLevel
{
  INFO,  // 普通信息
  ERROR, // 错误信息
  FATAL, // 致命信息
  DEBUG, // 调试信息
};

class Logger : noncopyable
{
public:
  // 获取日志唯一的实例对象
  static Logger &getinstance();
  // 设置日志级别
  void setLogLevel(int Level);
  // 写日志
  void log(std::string msg);

private:
  int logLevel_; // 把_放在后面是为了不和系统的发生冲突，因为系统在命名的时候放在前面

  Logger(){};
};
