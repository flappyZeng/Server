#include"log.h"
#include<map>
#include<functional>
#include"config.h"

//#define LogLvel::name (#name)

namespace server {
	
	//LogLevel里面的一个转化字符串函数logLevel -> str(logLevel)
	const char* LogLevel::toString(LogLevel::Level level) {
		switch (level) {
#define toStr(name)\
 case LogLevel::name: \
return #name; \
break;
			toStr(DEBUG);
			toStr(INFO);
			toStr(WARN);
			toStr(ERROR);
			toStr(FATAL);
#undef toStr
		default:
			return "UNKNOWN";
		}
		return "UNKNOWN";
	}

	LogLevel::Level LogLevel::fromString(const std::string& str){
		static std::unordered_map<std::string, LogLevel::Level> str_to_level = {
			{"DEBUG", LogLevel::DEBUG},
			{"INFO", LogLevel::INFO},
			{"WARN", LogLevel::WARN},
			{"ERROR", LogLevel::ERROR},
			{"FATAL", LogLevel::FATAL}
		};
		std::string query = str;
		std::transform(query.begin(), query.end(), query.begin(), ::toupper);
		if(str_to_level.find(query) != str_to_level.end()){
			return str_to_level[query];
		}
		else{
			return LogLevel::UNKNOWN;
		}
	}

	//一些定义的解析格式
	// %m --- 消息体
	// %p --- 日志级别
	// %r --- 启动后的时间
	// %c --- 日志名称
	// %t --- 线程id
	// %n --- 回车换行
	// %d --- 时间
	// %f --- 文件名
	// %F --- 协程号
	// %l --- 行号
	// %T --- 输出tab（\t)

//FormatItem的几个实例
	class MessageFormatItem :public FormatItem {   //输出字符串内容
	public:
		MessageFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getContent();
		}
	};

	class LevelFormatItem :public FormatItem {  //输出Dubug等级
	public:
		LevelFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << LogLevel::toString(level);
		}
	};

	class LogNameFormatItem : public FormatItem {  //输出日志名称
	public:
		LogNameFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getLogger()->getName();      //输出event的logger名称
		}
	};

	class ElapseFormatItem : public FormatItem {	//输出启动后的时间, 需要格式化
	public:
		ElapseFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getElapse();
		}
	};

	class ThreadIdFormatItem : public FormatItem { //输出线程id
	public:
		ThreadIdFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getThreadId();
		}
	};

	class ThreadNameFormatItem : public FormatItem { //输出线程名称
	public:
		ThreadNameFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getThreadName();
		}
	};

	class FiberIdFormatItem : public FormatItem { //输出协程给id
	public:
		FiberIdFormatItem(const std::string& str = " ") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getFiberId();
		}
	};

	class DateTimeFormatItem : public FormatItem { //输出时间
	public:
		DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S"){ 
			if(format != "")
				m_format = format;
		}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			time_t time = event->getTime();
			struct tm *t  = localtime(&time);
			//std::cout << t->tm_year<< " " << t->tm_yday << std::endl;
			//std::cout << time << std::endl;
			char buf[64];
			strftime(buf,64, m_format.c_str(), t);
			//std::cout << "buff" << m_format << std::endl;
			os << buf;
		}
	private:
		std::string m_format = "%Y:%m:%d %H:%M:%S";
	};

	class FileNameFormatItem : public FormatItem { //输出文件名
	public:
		FileNameFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getFile();
		}
	};

	class LineFormatItem : public FormatItem {		//输出行号
	public:
		LineFormatItem(const std::string& str = " ") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << event->getLine();
		}
	};

	class NewLineFormatItem :public FormatItem {  //输出换行符
	public:
		NewLineFormatItem(const std::string& str = " ") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << std::endl;
		}
	};

	class TabFormatItem :public FormatItem {  //输出tab
	public:
		TabFormatItem(const std::string& str = "") {}
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << "\t"; 
		}
	};
	class StringFormatItem : public FormatItem { //输出普通字符
	public:
		StringFormatItem(const std::string& str)
			:m_string(str) {
		};
		void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) override {
			os << m_string;
		}
	private:
		std::string m_string;
	};

	//Logger函数实现
	Logger::Logger(const std::string& name)
		:m_name(name),
		m_level(LogLevel::DEBUG) {
		m_formatter.reset(new LogFormatter("%d{%Y:%m:%d %H:%M:%S}%T%t%T%N%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
	}

	void Logger::log(LogLevel::Level level, LogEvent::ptr event) {
		if (level >= m_level) {  //log的级别要大于自身的级别
			auto self = shared_from_this();
			MutexType::Lock lock(m_mutex);
			if(!m_appenders.empty()){
				for (auto& i : m_appenders) {
					//MutexType::Lock llock(i->m_mutex);
					//std::cout << i->getFormatter()->getPattern() << std::endl;
					i->log(self, level, event);
				}
			}
			else if(m_root){
				m_root->log(level, event);
			}
		}
	}

	void Logger::debug(LogEvent::ptr event) {
		log(LogLevel::DEBUG, event);
	}

	void Logger::info(LogEvent::ptr event) {
		log(LogLevel::INFO, event);
	}

	void Logger::warn(LogEvent::ptr event) {
		log(LogLevel::WARN, event);
	}

	void Logger::error(LogEvent::ptr event) {
		log(LogLevel::ERROR, event);
	}

	void Logger::fatal(LogEvent::ptr event) {
		log(LogLevel::FATAL, event);
	}

	void Logger::addAppender(LogAppender::ptr appender) {
		//MutexType::Lock lock(m_mutex);
		if (!appender->getFormatter()) {
			MutexType::Lock lockl(appender->m_mutex);
			appender->m_formatter = m_formatter;  //保证至少有一个默认formatter, 这里使用logger是LogAppender的友元类这一特征
		}
		m_appenders.push_back(appender);
	}

	void Logger::delAppender(LogAppender::ptr appender) {
		MutexType::Lock lock(m_mutex);
		for (auto it = m_appenders.begin(); it != m_appenders.end(); ++it) {
			if (*it == appender) {
				m_appenders.erase(it);
				break;
			}
		}
	}
	void Logger::clearApender(){
		MutexType::Lock lock(m_mutex);
		m_appenders.clear();
	}
	
	void Logger::setFormatter(LogFormatter::ptr fmt) {
		MutexType::Lock lock(m_mutex);
		//std::cout << "change the format to " << fmt->getPattern() << std::endl;
		m_formatter = fmt;
		for(auto appender : m_appenders){
			//下游没有自定义的appender都要改成这个模式
			if(!appender->m_hasFormatter)
				appender->setFormatter(fmt);
		}
	}

	void Logger::setFormatter(const std::string fmt){
		LogFormatter::ptr new_fmt(new LogFormatter(fmt));
		if(!new_fmt->isError()){
			setFormatter(new_fmt);
		}else{
			//解析错误，控制台输出错误
			std::cout << "Logger setFormatter name = " << m_name
				<< " value= "<< fmt << " invalid formatter"
				<<std::endl;
		}
	}

	std::string Logger::toYamlString(){
		MutexType::Lock lock(m_mutex);
		YAML::Node node;
		node["name"] = m_name;
		if(m_level != LogLevel::UNKNOWN)
			node["level"] = LogLevel::toString(m_level);
		if(m_formatter){
			node["formatter"] = m_formatter->getPattern();
		}
		for(auto& ap : m_appenders){
			node["appenders"].push_back(YAML::Load(ap->toYamlString()));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}

	//StdoutLogApeender
	void StdoutLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
		if (level >= m_level) {
			//MutexType::Lock lock(m_mutex);
			std::cout << m_formatter->format(logger, level, event);

		}
	}

	std::string StdoutLogAppender::toYamlString(){
		MutexType::Lock lock(m_mutex);
		YAML::Node node;
		node["type"] =  "StdoutLogAppender";
		if(m_level != LogLevel::UNKNOWN)
			node["level"] = LogLevel::toString(m_level);
		if(m_hasFormatter && m_formatter){
			node["formatter"] = m_formatter->getPattern();
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}

	//FileLogAppender
	void FileLogAppender::log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) {
		if (level > m_level) {
			//每秒重开一次文件，防止文件被误删，在加锁后重开才对
			uint64_t now_time = time(0);
			if(now_time != m_nowtime){
				reopen();
				m_nowtime = now_time;
			}
			m_filestream << m_formatter->format(logger, level, event);
		}
	}

	std::string FileLogAppender::toYamlString(){
		MutexType::Lock lock(m_mutex);
		YAML::Node node;
		node["type"] =  "StdoutLogAppender";
		node["file"] = m_filename;
		if(m_level != LogLevel::UNKNOWN)
			node["level"] = LogLevel::toString(m_level);
		if(m_hasFormatter && m_formatter){
			node["formatter"] = m_formatter->getPattern();
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}

	bool FileLogAppender::reopen() {
		MutexType::Lock lock(m_mutex);
		if (m_filestream) {
			m_filestream.close();
		}
		m_filestream = std::ofstream(m_filename, std::stringstream::app);
		return !!m_filestream;  //直接用两个！！两次转换；
	}

	//LogFormatter
	std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) {
		std::stringstream ss;
		for (auto& item : m_items) {
			item->format(ss, logger, level, event);  //多态，指针，动态类型识别
		}
		return ss.str();  //将字符串流转换为字符串
	}

	//输出格式类型： %xxx, %xxx{xxx} %%, pattern解析
	void LogFormatter::init() { 
		//解析对应的pattern
		//str, format, type
		std::vector< std::tuple<std::string, std::string, int> >vec;
		std::string nstr;
		//std::cout << m_pattrern << std::endl;
		for (size_t i = 0; i < m_pattrern.size(); ++i) {
			if (m_pattrern[i] != '%') {  //不是百分号，就是普通的字符，存下来
				nstr.append(1, m_pattrern[i]);
				continue;
			}
			//等于百分号，开始解析对应的格式

			size_t n = i + 1;
			size_t fmt_begin = 0;
			int fmt_statue = 0;

			//下一个还是%代表就是输入了一个百分号
			if (n < m_pattrern.size()) {
				if (m_pattrern[n] == '%'){
					nstr.append(1, '%');
					continue;
				}
			}

			std::string str;
			std::string fmt;
			while (n < m_pattrern.size()) {
				//空格，代表当前这个%代表的字段解析完毕了,不是字母，也不是{,也不是}, 这个地方要加一个状态控制，解析{之后的内容要无视%。
				if (!isalpha(m_pattrern[n]) && m_pattrern[n] != '{' && m_pattrern[n] != '}' && fmt_statue == 0) {
					break;
				}
				if (fmt_statue == 0) {
					//解析到当前的是{,代表是第二种格式，%xxx{xxx}， 前面的名称已经解析好了
					if (m_pattrern[n] == '{') {
						str = m_pattrern.substr(i + 1, n - i - 1);  //名称
						fmt_statue = 1;  //开始解析内容格式
						fmt_begin = n;
						++n;
						continue;
					}
				}
				if (fmt_statue == 1) {
					//解析到当前的是},代表是第二种格式，%xxx{xxx}， {之后， }之前的就是内容
					if (m_pattrern[n] == '}') {
						fmt = m_pattrern.substr(fmt_begin + 1, n - fmt_begin - 1);
						//std::cout << "after{ ： " << fmt << std::endl;
						fmt_statue = 2;
						break;
					}
				}
				++n;  //这里应该补一个++n的操作;
			}


			//代表没有出现{},就是完整的格式
			if (fmt_statue == 0) {
				//解析%前还有一串字符，保存一下
				if (!nstr.empty()) {
					vec.push_back(std::make_tuple(nstr, "", 0));
					nstr.clear();
				}
				str = m_pattrern.substr(i + 1, n - i - 1);
				vec.push_back(std::make_tuple(str, fmt, 1));
				i = n - 1;	
			}
			//只有{没有解析到}.解析出错
			else if (fmt_statue == 1) {
				std::cout << "pattern parse error: " << m_pattrern << " - " << m_pattrern.substr(i) << std::endl;
				vec.push_back(std::make_tuple("<<pattern_error>>", fmt, 0));
				m_error = true;
				i = n - 1;
			}
			//解析成功
			else if (fmt_statue == 2) {
				//std::cout << "fmt_statue = 2" << std::endl;
				//解析%前还有一串字符，保存一下
				if (!nstr.empty()) {
					vec.push_back(std::make_tuple(nstr, "", 0));
					nstr.clear();
				}
				vec.push_back(std::make_tuple(str, fmt, 1));
				i = n ;
			}
		}
		//解析到字符拆串尾部 
		if (!nstr.empty()) {
			vec.push_back(std::make_tuple(nstr, "", 0));
		}

		static std::unordered_map<std::string, std::function<FormatItem::ptr(const std::string & str)> >s_format_items = {
			/*
#define XX(str, C) \
			{#str, [](const std::string& fmt) {return FormatItem::ptr(new C(fmt));}}
			XX(m ,MessageFormatItem),
			XX(p, LevelFormatItem),
			XX(r, ElapseFormatItem),
			XX(c, LogNameFormatItem),
			XX(t, ThreadIdFormatItem),
			XX(n, NewLineFormatItem),
			XX(d, DateTimeFormatItem),
			XX(f, FileNameFormatItem),
			XX(l, LineFormatItem)
#undef XX
*/			{"m", [](const std::string& fmt) {return FormatItem::ptr(new MessageFormatItem(fmt)); }},
			{"p", [](const std::string& fmt) {return FormatItem::ptr(new LevelFormatItem(fmt)); }},
			{"r", [](const std::string& fmt) {return FormatItem::ptr(new ElapseFormatItem(fmt)); }},
			{"c", [](const std::string& fmt) {return FormatItem::ptr(new LogNameFormatItem(fmt)); }},
			{"N", [](const std::string& fmt) {return FormatItem::ptr(new ThreadNameFormatItem(fmt)); }},
			{"t", [](const std::string& fmt) {return FormatItem::ptr(new ThreadIdFormatItem(fmt)); }},
			{"n", [](const std::string& fmt) {return FormatItem::ptr(new NewLineFormatItem(fmt)); }},
			{"d", [](const std::string& fmt) {return FormatItem::ptr(new DateTimeFormatItem(fmt)); }},
			{"f", [](const std::string& fmt) {return FormatItem::ptr(new FileNameFormatItem(fmt)); }},
			{"F", [](const std::string& fmt) {return FormatItem::ptr(new FiberIdFormatItem(fmt)); }},
			{"l", [](const std::string& fmt) {return FormatItem::ptr(new LineFormatItem(fmt)); }},
			{"T", [](const std::string& fmt) {return FormatItem::ptr(new TabFormatItem(fmt)); }}

		};

		//std::cout << vec.size() << std::endl;
		for (auto t : vec) {
			if (std::get<2>(t) == 0) {
				m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(t))));
			}
			else {
				auto it = s_format_items.find(std::get<0>(t));
				if (it == s_format_items.end()) {
					m_items.push_back(FormatItem::ptr(new StringFormatItem("<<Error_format " + std::get<0>(t) + ">>")));
					m_error = true;
				}
				else{
					m_items.push_back(it->second(std::get<1>(t)));
				}
			}
			//std::cout <<"{" << std::get<0>(t) << "}-{" << std::get<1>(t) << "}-{" << std::get<2>(t) <<"}" <<std::endl;
		}
		//std::cout << m_items.size() << std::endl;

	}

	void LogAppender::setFormatter(LogFormatter::ptr val){
		MutexType::Lock lock(m_mutex);
		m_formatter = val;
		m_hasFormatter = true;
	}
	//logApeender函数实现
	void LogAppender::setFormatter(const std::string& fmt){
		LogFormatter::ptr new_fmt(new LogFormatter(fmt));
		if(!new_fmt->isError()){
			MutexType::Lock lock(m_mutex);
			m_formatter = new_fmt;
			m_hasFormatter = true;
		}else{
			std::cout << "LogAppender setFormatter "
				<< " value= "<< fmt << " invalid formatter"
				<<std::endl;
		}
	}
	LogFormatter::ptr LogAppender::getFormatter(){
		//MutexType::Lock lock(m_mutex);
		return m_formatter;
	}
	

	//logEvent实现
	LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char* file, 
						int32_t line, uint32_t elapse, uint32_t threadId, uint32_t fiberId, uint64_t time, const std::string& thread_name)
		: m_file(file)
		, m_line(line)
		, m_elapse(elapse)
		, m_threadId(threadId)
		, m_threadName(thread_name)
		, m_fiberId(fiberId)
		, m_time(time)
		, m_logger(logger)
		, m_level(level) {
	}

	void LogEvent::format(const char* fmt, ...){
		va_list va;
		va_start(va, fmt);
		format(fmt, va);
		va_end(va);
	}
	void LogEvent::format(const char* fmt, va_list al){
		char* buf = nullptr;
		int len = vasprintf(&buf, fmt, al);
		if(len != -1){
			m_ss << std::string(buf, len);
		}
	}
	LogEventWrap::~LogEventWrap(){
		m_event->getLogger()->log(m_event->getLevel(), m_event);
	}

	//LoggerManager相关函数
	LoggerManager::LoggerManager()
		:m_root(new Logger()){
		m_root->addAppender(LogAppender::ptr(new StdoutLogAppender())); //root里面加了一个appender
		m_loggers[m_root->getName()] = m_root;  //将自己添加到对应的map里面
		init();
	}

	Logger::ptr LoggerManager::getLogger(const std::string& name){
		MutexType::Lock lock(m_mutex);
		auto it = m_loggers.find(name);
		if(it != m_loggers.end())
			return it->second;
		Logger::ptr logger(new Logger(name));
		logger->m_root = m_root;  //避免使用该logger的时候没有定义loggerAppener时，使用默认的m_root的appener,设定为默认名字为root，输出到控制台
		m_loggers[name] = logger;
		return logger;
	}

	std::string LoggerManager::toYamlString(){
		MutexType::Lock lock(m_mutex);
		YAML::Node node;
		for(auto& log: m_loggers){
			node.push_back(YAML::Load(log.second->toYamlString()));
		}
		std::stringstream ss;
		ss << node;
		return ss.str();
	}
	
	//读取配置文件相关设置

	//读取配置文件需要的结构体
	struct LogAppenderDefine{
		int type = 0; // 1表示FileLogAppender, 2.表示StdoutLogApeender, 
		LogLevel::Level level = LogLevel::DEBUG;
		std::string formatter;
		std::string file;

		//等号运算符
		bool operator==(const LogAppenderDefine& oth) const{
			return type == oth.type
				&& level == oth.level
				&& formatter == oth.formatter
				&& file == oth.file;
		}
	};

	struct LogDefine{
		std::string name;
		LogLevel::Level level = LogLevel::DEBUG;
		std::string formatter;
		std::vector<LogAppenderDefine> appenders;

		bool operator==(const LogDefine& oth) const{
			return name == oth.name
				&& level == oth.level
				&& formatter == oth.formatter
				&& appenders == oth.appenders;
		}

		//因为使用了set<LogDefine>，要重载小于号<才行
		bool operator<(const LogDefine& oth) const{
			return name < oth.name;
		}
	};

	//Config 的LogDefine偏特化
	template<>
	class LexicalCast<std::string, std::set<LogDefine> >{
	public:
		std::set<LogDefine> operator()(const std::string & val){
			YAML::Node node = YAML::Load(val);
			std::set<LogDefine> st;
			std::stringstream ss;
			for(size_t i = 0; i < node.size(); ++i){
				auto n = node[i];
				if(!n["name"].IsDefined()){ //log name未定义，说明这个配置文件有错。
					std::cout << "log config error, name is error" << n << std::endl;
					continue;
				}
				LogDefine log;
				//这里后期可能要加内容校验
				log.name = n["name"].as<std::string>();
				//UNKNOWN代表最低级，还是不要改成继承logger吧
				// if(n["level"].IsDefined()){
				// 	LogLevel::Level level = LogLevel::fromString(n["level"].as<std::string>());
				// 	if(level != LogLevel::UNKNOWN){
				// 		log.level = level;
				// 	}
				// }

				log.level = LogLevel::fromString(n["level"].IsDefined() ? n["level"].as<std::string>(): "");
				if(n["formatter"].IsDefined()){
					log.formatter = n["formatter"].as<std::string>();
				}
				if(n["appenders"].IsDefined()){
					for(size_t j = 0; j < n["appenders"].size(); ++j){
						auto sap = n["appenders"][j];
						if(!sap["type"].IsDefined()){
							std::cout << "log config error, appender type is error" << sap << std::endl;
							continue;
						}
						LogAppenderDefine logad;
						std::string type = sap["type"].as<std::string>();
						if(type == "FileLogAppender"){
							logad.type = 1;
							if(!sap["file"].IsDefined()){
								std::cout << "log config error, appender filename is error" << sap << std::endl;
								continue;
							}
							logad.file = sap["file"].as<std::string>();
						}else if(type == "StdoutLogAppender"){
							logad.type = 2;
						}else{
							std::cout << "log config error, appender type is error" << sap << std::endl;
							continue;
						}
						//formatter以及level有默认值，可以不存在。

						// if(sap["level"].IsDefined()){
						// 	LogLevel::Level level = LogLevel::fromString(sap["level"].as<std::string>());
						// 	if(level != LogLevel::UNKNOWN){
						// 		logad.level = level;
						// 	}else{
						// 		logad.level = log.level;
						// 	}
						// }else{
						// 	logad.level = log.level;   //没有定义就跟随logger
						// }
						logad.level = LogLevel::fromString(sap["level"].IsDefined() ? sap["level"].as<std::string>(): "");
						//这里把logad写成了log，粗心，下次要记住
						if(sap["formatter"].IsDefined()){
							logad.formatter = sap["formatter"].as<std::string>();
						}
						log.appenders.push_back(logad);
					}
				}
				st.insert(log);
			}
			return st;
		}
	};	

	template<>
	class LexicalCast<std::set<LogDefine>, std::string >{
	public:
		std::string operator()(const std::set<LogDefine> v){
			YAML::Node node;
            for(auto& i : v){
                YAML::Node n;
				n["name"] = i.name;
				if(i.level != LogLevel::UNKNOWN)
					n["level"] = LogLevel::toString(i.level);
				if(!i.formatter.empty())
					n["formatter"] = i.formatter;
				for(auto& ap : i.appenders){
					YAML::Node nap;
					if(ap.type == 1){
						nap["type"] = "FileLogAppender";
						nap["file"] = ap.file;
					}else{
						nap["type"] = "StdoutLogAppender";
					}
					if(ap.level != LogLevel::UNKNOWN)
						nap["level"] = LogLevel::toString(ap.level);
					if(!ap.formatter.empty())
						nap["formatter"] = ap.formatter;
					n["appenders"].push_back(nap);
				}
				node.push_back(n);
            }
            std::stringstream ss;
            ss << node;
            return ss.str();
		}
	};



	//配置读取器
	ConfigVar<std::set<LogDefine> >::ptr g_log_defines = Config::lookup("logs", std::set<LogDefine>(), "logs configs");

	struct LogIniter{
		LogIniter(){
			//给配置读取器增加监听器
			g_log_defines->addListener([](const std::set<LogDefine>& old_value, const std::set<LogDefine>& new_value) {
				//新增
				SERVER_LOG_INFO(SERVER_LOG_ROOT()) << "on logger config changed";
				for(auto& i : new_value){
					auto it = old_value.find(i);
					Logger::ptr logger;
					if(it == old_value.end()){
						//新增
						logger = SERVER_LOG_NAME(i.name);
					}
					else if(!(i == *it)){ //没有重载!=运算符
						//修改
						logger = SERVER_LOG_NAME(i.name);
					}					
					logger->setLevel(i.level);
					if(!i.formatter.empty()){
						logger->setFormatter(i.formatter);
					}
					logger->clearApender();
					for(auto& appender : i.appenders){
						LogAppender::ptr ap;
						if(appender.type == 1) {
							//FileLogAppener
							ap.reset(new FileLogAppender(appender.file));
						}else if(appender.type == 2){
							//StdoutLogAppender
							ap.reset(new StdoutLogAppender());
						}
						ap->setLevel(appender.level);
						//如果formatter为空的话就没必要设置了，让他的formatter等于logger的formatter
						if(appender.formatter != "")
							ap->setFormatter(appender.formatter);
						logger->addAppender(ap);
					}	
				}
				for(auto i : old_value){
					auto it = new_value.find(i);
					if(it == new_value.end()){
						//删除
						//实际操作时不一定是删除，因为删除的时候可能会出现配置出错，一般的做法就是让这个logger失效（比如将日志级别设置成极高）。
						auto logger = SERVER_LOG_NAME(i.name);
						//把他的appender清空就ok了,把level也设置的高一点
						logger->setLevel((LogLevel::Level)100);
						logger->clearApender();
					}
				}
			});
		}
	};

	//静态初始化监听器
	static LogIniter __log_init;

	//待实现
	void LoggerManager::init(){

	}

}

