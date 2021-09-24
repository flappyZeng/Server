#pragma once

#include<iostream>
#include<string>
#include<stdint.h>
#include<vector>
#include<memory>  //提供智能指针
#include<list>
#include<tuple>
#include<sstream> //提供stringstream;
#include<fstream> //文件流
#include<time.h>  //处理时间
#include<stdarg.h>  //提供可变参数相关的方法，例如va_start
#include<map>
#include"singleton.h"  //提供logger的单例
#include"utils.h"
#include"thread.h"  //提供线程和互斥量
#include"fiber.h"


//下方使用了一个宏__FILE__，默认为系统全路径，但是实际工程中一般要求避免泄漏全部路径，需要修改__FILE__输出相对路径，设置方法在CMAKE里面重定义该宏

//定义一些宏，简化用户使用难度
#define SERVER_LOG_LEVEL(logger, level) \
	if(logger->getLevel() <= level) \
		server::LogEventWrap(server::LogEvent::ptr(new server::LogEvent(logger, level, \
			__FILE__, __LINE__, 0, server::GetThreadId(), \
				server::Fiber::GetFiberId(), time(0), server::Thread::GetName()))).getSS()


#define SERVER_LOG_DEBUG(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::DEBUG)
#define SERVER_LOG_INFO(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::INFO)
#define SERVER_LOG_WARN(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::WARN)
#define SERVER_LOG_ERROR(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::ERROR)
#define SERVER_LOG_FATAL(logger) SERVER_LOG_LEVEL(logger, server::LogLevel::FATAL)


//定义格式化输出相关的宏（例如printf的效果）
#define SERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		server::LogEventWrap(server::LogEvent::ptr(new server::LogEvent(logger, level, \
			__FILE__, __LINE__, 0, server::GetThreadId(), \
				server::Fiber::GetFiberId(), time(0), server::Thread::GetName()))).getEvent()-> format(fmt, ##__VA_ARGS__)

//下面的__VA_ARGS__前面加##是为了处理无后续参数的情况
#define SERVER_LOG_FMT_DEBUG(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::DEBUG, , fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_INFO(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_WARN(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::WARN, fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_ERROR(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::ERROR, fmt,  ##__VA_ARGS__)
#define SERVER_LOG_FMT_FATAL(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::FATAL, fmt, ##__VA_ARGS__)


//返回logger单例里面的root指针
#define SERVER_LOG_ROOT() server::loggerMgr::getInstance()->getRoot()
//返回对应类型的logger
#define SERVER_LOG_NAME(name) server::loggerMgr::getInstance()->getLogger(name)

//涉及到非原子读写的方法都要加上互斥量
namespace server {

	class Logger;  //前向声明logger
	class LoggerManager; //前向声明loggerManager，使之作为logger的友元类

	//日志级别定义
	class LogLevel {
	public:
		enum Level {
			UNKNOWN,
			DEBUG = 1,
			INFO = 2,
			WARN = 3,
			ERROR = 4,
			FATAL = 5,
			
		};
		static const char* toString(Level level);
		static LogLevel::Level fromString(const std::string& str);
	};

	//规格化日志文件, 日志事件
	class  LogEvent {
	public:
		typedef std::shared_ptr<LogEvent> ptr;
		LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level,const char* file, int32_t line, uint32_t elapse,
			uint32_t threadId, uint32_t fiberId, uint64_t time, const std::string& thread_name);

		const char* getFile() const { return m_file; }
		int32_t getLine() const { return m_line; }
		uint32_t getElapse() const { return m_elapse; }
		uint32_t getThreadId() const { return m_threadId; }
		uint32_t getFiberId() const { return m_fiberId; }
		uint64_t getTime() const { return m_time; }
		const std::string getContent() const { return m_ss.str(); }
		const std::string& getThreadName() const {return m_threadName;}
		std::stringstream& getSS()  {return m_ss;}
		std::shared_ptr<Logger> getLogger() const {return m_logger;}
		LogLevel::Level getLevel() const {return m_level;}
		void format(const char* fmt, ...);
		void format(const char* fmt, va_list al);

	private:
		const char* m_file = nullptr;		//文件名
		int32_t m_line = 0;					//行号
		uint32_t m_elapse = 0;				//程序启动开始到现在的毫秒数
		uint32_t m_threadId = 0;			//线程id
		uint32_t m_fiberId = 0;				//协程id
		uint64_t m_time = 0;				//时间戳
		std::stringstream m_ss;				//消息流
		std::shared_ptr<Logger> m_logger;   //日志器指针
		LogLevel::Level m_level; 			//日志级别
		std::string m_threadName; 			//线程名称
	};

	//统一对外接口
	class LogEventWrap{
	public:
		LogEventWrap(LogEvent::ptr event): m_event(event) {};
		std::stringstream& getSS()  {
			return m_event->getSS();
		}
		LogEvent::ptr getEvent() const {return m_event;}
		~LogEventWrap();
	private:
		LogEvent::ptr m_event;
	};
	
	//用来解析不同格式的日志项
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() {};
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;  //输出对应的项
	};

	//日志格式器
	class LogFormatter {
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		//日志的输出格式 %t		%threadId %m%n （log4j)
		LogFormatter(const std::string& pattren) :m_pattrern(pattren) {init();}
		std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);    //格式化输入的logEvent为一个string输出

		const std::string getPattern() const {return m_pattrern;}
		bool isError() const {return m_error;}
	private:
		void init();
	private:
		//根据pattern的格式来解析不同的item
		std::string m_pattrern;  //日志结构
		std::vector<FormatItem::ptr>m_items;
		bool m_error = false;   //判断格式是否有误
	};

	//日志输出目的地
	class LogAppender {
	friend class Logger;   //设置为logger的友元类的目的是为了在logger中设置appender的formatter
	public:
		typedef SpinLock MutexType;
		typedef std::shared_ptr<LogAppender> ptr;
		virtual ~LogAppender() {}  //这里定义成虚函数的原因是需要继承。

		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;  //纯虚函数。

		void setLevel(LogLevel::Level level) {m_level = level;}

		void setFormatter(LogFormatter::ptr val);
		void setFormatter(const std::string& fmt);
		LogFormatter::ptr getFormatter();

		//将appeder序列化为Yaml字符串
		virtual std::string toYamlString() = 0;

	protected:
		bool m_hasFormatter = false;  //定义有没有formatter;
		LogLevel::Level m_level = LogLevel::DEBUG;  //定义这个logappend对应的日志级别, 得初始化
		LogFormatter::ptr m_formatter;
		MutexType m_mutex;    //互斥量
	};

	//日志器类
	class Logger : public std::enable_shared_from_this<Logger>{
	friend class LoggerManager;
	public:
		typedef SpinLock MutexType;
		typedef std::shared_ptr<Logger> ptr;

		Logger(const std::string& name = "root");
		void log(LogLevel::Level level, LogEvent::ptr event);

		//定义loger操作输出；
		void debug(LogEvent::ptr event);
		void info(LogEvent::ptr event);
		void warn(LogEvent::ptr event);
		void error(LogEvent::ptr event);
		void fatal(LogEvent::ptr event);

		//appender添加/删除
		void addAppender(LogAppender::ptr appender);
		void delAppender(LogAppender::ptr appender);
		void clearApender();

		//获取日志的debug级别
		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level level) { m_level = level; }

		//获取logger内部变量
		const std::string& getName() const { return m_name; }

		//设置日志输出的格式：这个地方要设置所有的Appender的formatter要为新的
		void setFormatter(LogFormatter::ptr fmt);
		void setFormatter(const std::string fmt);
		LogFormatter::ptr getFormatter() const {return m_formatter;}

		//将logger对象序列化为Yaml字符串
		std::string toYamlString();

	private:
		std::string m_name;							//日志的名字
		LogLevel::Level m_level;					//定义日志级别；
		std::list<LogAppender::ptr> m_appenders;	//appender集合	
		LogFormatter::ptr m_formatter;
		Logger::ptr m_root;
		MutexType m_mutex;  //互斥量
	};

	//定义日志输出到控制台的appender
	class StdoutLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;  //实现父类的纯虚函数

		//将appeder序列化为Yaml字符串
		virtual std::string toYamlString() override;
	};

	// 定义输出到文件的appender
	class FileLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		FileLogAppender(const std::string& filename) :m_filename(filename) {reopen();};
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override; //实现父类的纯虚函数
		
		//将appeder序列化为Yaml字符串
		virtual std::string toYamlString() override;

		bool reopen();   //文件重新打开；
	private:
		std::string m_filename;
		std::ofstream m_filestream;
		uint64_t m_nowtime = 0;
	};

	//工厂模式应用，logger管理器：
	class LoggerManager{
	public:
		typedef SpinLock MutexType;
		LoggerManager();
		Logger::ptr getLogger(const std::string& name);
		void init();

		//获取默认的logger
		Logger::ptr getRoot() const {return m_root; }
		std::string toYamlString();
	private:
		std::map<std::string, Logger::ptr > m_loggers;
		Logger::ptr m_root;
		MutexType m_mutex;  //互斥量，防止map访问出错
	};
	typedef Singleton<LoggerManager> loggerMgr;
	typedef SingletonPtr<LoggerManager> loggerMgrPtr;
	//static ConfigVar<std::set<LogDefine> >::ptr g_log_defines;
};