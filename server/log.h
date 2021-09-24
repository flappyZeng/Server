#pragma once

#include<iostream>
#include<string>
#include<stdint.h>
#include<vector>
#include<memory>  //�ṩ����ָ��
#include<list>
#include<tuple>
#include<sstream> //�ṩstringstream;
#include<fstream> //�ļ���
#include<time.h>  //����ʱ��
#include<stdarg.h>  //�ṩ�ɱ������صķ���������va_start
#include<map>
#include"singleton.h"  //�ṩlogger�ĵ���
#include"utils.h"
#include"thread.h"  //�ṩ�̺߳ͻ�����
#include"fiber.h"


//�·�ʹ����һ����__FILE__��Ĭ��Ϊϵͳȫ·��������ʵ�ʹ�����һ��Ҫ�����й©ȫ��·������Ҫ�޸�__FILE__������·�������÷�����CMAKE�����ض���ú�

//����һЩ�꣬���û�ʹ���Ѷ�
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


//�����ʽ�������صĺ꣨����printf��Ч����
#define SERVER_LOG_FMT_LEVEL(logger, level, fmt, ...) \
	if(logger->getLevel() <= level) \
		server::LogEventWrap(server::LogEvent::ptr(new server::LogEvent(logger, level, \
			__FILE__, __LINE__, 0, server::GetThreadId(), \
				server::Fiber::GetFiberId(), time(0), server::Thread::GetName()))).getEvent()-> format(fmt, ##__VA_ARGS__)

//�����__VA_ARGS__ǰ���##��Ϊ�˴����޺������������
#define SERVER_LOG_FMT_DEBUG(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::DEBUG, , fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_INFO(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::INFO, fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_WARN(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::WARN, fmt, ##__VA_ARGS__)
#define SERVER_LOG_FMT_ERROR(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::ERROR, fmt,  ##__VA_ARGS__)
#define SERVER_LOG_FMT_FATAL(logger, fmt, ...) SERVER_LOG_FMT_LEVEL(logger, server::LogLevel::FATAL, fmt, ##__VA_ARGS__)


//����logger���������rootָ��
#define SERVER_LOG_ROOT() server::loggerMgr::getInstance()->getRoot()
//���ض�Ӧ���͵�logger
#define SERVER_LOG_NAME(name) server::loggerMgr::getInstance()->getLogger(name)

//�漰����ԭ�Ӷ�д�ķ�����Ҫ���ϻ�����
namespace server {

	class Logger;  //ǰ������logger
	class LoggerManager; //ǰ������loggerManager��ʹ֮��Ϊlogger����Ԫ��

	//��־������
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

	//�����־�ļ�, ��־�¼�
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
		const char* m_file = nullptr;		//�ļ���
		int32_t m_line = 0;					//�к�
		uint32_t m_elapse = 0;				//����������ʼ�����ڵĺ�����
		uint32_t m_threadId = 0;			//�߳�id
		uint32_t m_fiberId = 0;				//Э��id
		uint64_t m_time = 0;				//ʱ���
		std::stringstream m_ss;				//��Ϣ��
		std::shared_ptr<Logger> m_logger;   //��־��ָ��
		LogLevel::Level m_level; 			//��־����
		std::string m_threadName; 			//�߳�����
	};

	//ͳһ����ӿ�
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
	
	//����������ͬ��ʽ����־��
	class FormatItem {
	public:
		typedef std::shared_ptr<FormatItem> ptr;
		virtual ~FormatItem() {};
		virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;  //�����Ӧ����
	};

	//��־��ʽ��
	class LogFormatter {
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		//��־�������ʽ %t		%threadId %m%n ��log4j)
		LogFormatter(const std::string& pattren) :m_pattrern(pattren) {init();}
		std::string format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event);    //��ʽ�������logEventΪһ��string���

		const std::string getPattern() const {return m_pattrern;}
		bool isError() const {return m_error;}
	private:
		void init();
	private:
		//����pattern�ĸ�ʽ��������ͬ��item
		std::string m_pattrern;  //��־�ṹ
		std::vector<FormatItem::ptr>m_items;
		bool m_error = false;   //�жϸ�ʽ�Ƿ�����
	};

	//��־���Ŀ�ĵ�
	class LogAppender {
	friend class Logger;   //����Ϊlogger����Ԫ���Ŀ����Ϊ����logger������appender��formatter
	public:
		typedef SpinLock MutexType;
		typedef std::shared_ptr<LogAppender> ptr;
		virtual ~LogAppender() {}  //���ﶨ����麯����ԭ������Ҫ�̳С�

		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;  //���麯����

		void setLevel(LogLevel::Level level) {m_level = level;}

		void setFormatter(LogFormatter::ptr val);
		void setFormatter(const std::string& fmt);
		LogFormatter::ptr getFormatter();

		//��appeder���л�ΪYaml�ַ���
		virtual std::string toYamlString() = 0;

	protected:
		bool m_hasFormatter = false;  //������û��formatter;
		LogLevel::Level m_level = LogLevel::DEBUG;  //�������logappend��Ӧ����־����, �ó�ʼ��
		LogFormatter::ptr m_formatter;
		MutexType m_mutex;    //������
	};

	//��־����
	class Logger : public std::enable_shared_from_this<Logger>{
	friend class LoggerManager;
	public:
		typedef SpinLock MutexType;
		typedef std::shared_ptr<Logger> ptr;

		Logger(const std::string& name = "root");
		void log(LogLevel::Level level, LogEvent::ptr event);

		//����loger���������
		void debug(LogEvent::ptr event);
		void info(LogEvent::ptr event);
		void warn(LogEvent::ptr event);
		void error(LogEvent::ptr event);
		void fatal(LogEvent::ptr event);

		//appender���/ɾ��
		void addAppender(LogAppender::ptr appender);
		void delAppender(LogAppender::ptr appender);
		void clearApender();

		//��ȡ��־��debug����
		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level level) { m_level = level; }

		//��ȡlogger�ڲ�����
		const std::string& getName() const { return m_name; }

		//������־����ĸ�ʽ������ط�Ҫ�������е�Appender��formatterҪΪ�µ�
		void setFormatter(LogFormatter::ptr fmt);
		void setFormatter(const std::string fmt);
		LogFormatter::ptr getFormatter() const {return m_formatter;}

		//��logger�������л�ΪYaml�ַ���
		std::string toYamlString();

	private:
		std::string m_name;							//��־������
		LogLevel::Level m_level;					//������־����
		std::list<LogAppender::ptr> m_appenders;	//appender����	
		LogFormatter::ptr m_formatter;
		Logger::ptr m_root;
		MutexType m_mutex;  //������
	};

	//������־���������̨��appender
	class StdoutLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;  //ʵ�ָ���Ĵ��麯��

		//��appeder���л�ΪYaml�ַ���
		virtual std::string toYamlString() override;
	};

	// ����������ļ���appender
	class FileLogAppender : public LogAppender {
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		FileLogAppender(const std::string& filename) :m_filename(filename) {reopen();};
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override; //ʵ�ָ���Ĵ��麯��
		
		//��appeder���л�ΪYaml�ַ���
		virtual std::string toYamlString() override;

		bool reopen();   //�ļ����´򿪣�
	private:
		std::string m_filename;
		std::ofstream m_filestream;
		uint64_t m_nowtime = 0;
	};

	//����ģʽӦ�ã�logger��������
	class LoggerManager{
	public:
		typedef SpinLock MutexType;
		LoggerManager();
		Logger::ptr getLogger(const std::string& name);
		void init();

		//��ȡĬ�ϵ�logger
		Logger::ptr getRoot() const {return m_root; }
		std::string toYamlString();
	private:
		std::map<std::string, Logger::ptr > m_loggers;
		Logger::ptr m_root;
		MutexType m_mutex;  //����������ֹmap���ʳ���
	};
	typedef Singleton<LoggerManager> loggerMgr;
	typedef SingletonPtr<LoggerManager> loggerMgrPtr;
	//static ConfigVar<std::set<LogDefine> >::ptr g_log_defines;
};