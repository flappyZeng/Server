#include"../log.h"
#include"../utils.h"

int main() {
	server::Logger::ptr log(new server::Logger());
	//log->addApender(server::LogAppender::ptr(new server::FileLogAppender("./log.txt")));
	log->addAppender(server::LogAppender::ptr(new server::StdoutLogAppender()));
	log->setLevel(server::LogLevel::ERROR);

	//设置日志格式(正常使用)
	//server::LogFormatter::ptr new_format(new server::LogFormatter("%d%T%p%T%m\n"));
	//log->setFormatter(new_format);

	//正常使用
	//server::LogEvent::ptr event(new server::LogEvent(log,server::LogLevel::DEBUG, __FILE__, __LINE__, 0, server::GetThreadId(), server::GetFiberId(), time(0)));
	//event->getSS() << "hello my server!";  //注入日志消息内容
	//log->log(server::LogLevel::DEBUG, event);

	//使用定义的宏 正常使用
	/*
	SERVER_LOG_DEBUG(log) << "test log debug";
	SERVER_LOG_INFO(log) << "test log info";
	SERVER_LOG_WARN(log) << "test log warn";
	SERVER_LOG_ERROR(log) << "test log error";
	*/

	SERVER_LOG_FATAL(log) << "test log fatal";
	
	SERVER_LOG_FMT_WARN(log, "test fmt %s on the fmt %d ss ", "error fatal", 100);
	
	//测试loggerManager, 正确
	auto logg = server::loggerMgr::getInstance()->getLogger("no one");
	SERVER_LOG_INFO(logg) << "test loggrmgr";
	
	return 0;
}
