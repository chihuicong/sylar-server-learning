#include "log.h"
#include <map>
#include <iostream>
#include <functional>
#include <time.h>
#include <string.h>
#include "config.h"
#include "util.h"

namespace sylar{

LogEvent::LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level, const char *file, int32_t line, uint32_t elapse, uint32_t thread_id, uint32_t fiber_id, uint64_t time)
    :m_file(file)
    ,m_line(line)
    ,m_elapse(elapse)
    ,m_threadId(thread_id)
    ,m_fiberId(fiber_id)
    ,m_time(time)
    ,m_logger(logger)
    ,m_level(level)
{

}

Logger::Logger(const std::string &name):m_name(name),m_level(LogLevel::DEBUG)
{
    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%m:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
//    std::cout << "m_pattern: " << m_formatter->getPattern() << std::endl;
//    m_formatter.reset(new LogFormatter("%d{%Y-%m-%d %H:%M:%S}%T%t%T%F%T[%p]%T[%c]%T%f:%l%T%m%n"));
//    if(name == "root"){
//        m_appenders.push_back(LogAppender::ptr(new StdoutLogAppender));
//    }
}

std::string Logger::toYamlString(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["name"] = m_name;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    for(auto& i : m_appenders){
        node["appenders"].push_back(YAML::Load(i->toYamlSreing()));
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void Logger::setFormatter(const std::string &val)
{

    sylar::LogFormatter::ptr new_val(new sylar::LogFormatter(val));
    if(new_val->isError()){
        std::cout << "Logger setFormatter name = " << m_name
                  <<" value = " << val << "invalid formatter"
                  << std::endl;
        return;
    }
    //m_formatter = new_val;
    setFormatter(new_val);
//    m_formatter.reset(new sylar::LogFormatter(val));
}

void Logger::setFormatter(LogFormatter::ptr val)
{
    MutexType::Lock lock(m_mutex);
    m_formatter = val;

    for(auto& i : m_appenders) {
        MutexType::Lock ll(i->m_mutex);
        if(!i->m_hasFormatter) {
            i->m_formatter = m_formatter;
        }
    }
}

LogFormatter::ptr Logger::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

const char* LogLevel::ToString(LogLevel::Level level)
{
    switch (level) {
#define XXX(name) \
    case LogLevel::name: \
        return #name; \
        break;
    XXX(DEBUG);
    XXX(INFO);
    XXX(WARN);
    XXX(ERROR);
    XXX(FATAL);
    default:
        return "UNKNOW";
    }
    return "UNKNOW";
}

LogLevel::Level LogLevel::FromString(const std::string &str)
{
#define XXXX(level, v) \
    if(str == #v){ \
        return LogLevel::level; \
    }

    XXXX(DEBUG, debug);
    XXXX(INFO, info);
    XXXX(WARN, warn);
    XXXX(ERROR, error);
    XXXX(FATAL, fatal);

    XXXX(DEBUG, DEBUG);
    XXXX(INFO, INFO);
    XXXX(WARN, WARN);
    XXXX(ERROR, ERROR);
    XXXX(FATAL, FATAL);
    return LogLevel::UNKNOW;
#undef XXXX
}


void LogEvent::format(const char *fmt,...)
{
    va_list al;
    va_start(al,fmt);
    format(fmt,al);
    va_end(al);
}

void LogEvent::format(const char *fmt, va_list al)
{
    char* buf = nullptr;
    int len = vasprintf(&buf, fmt, al);
    if(len != -1){
//        std::cout << "format" << std::endl; // 添加备注，删除
        m_ss << std::string(buf, len);
        free(buf);
    }
}

LogEventWrap::LogEventWrap(LogEvent::ptr e):m_event(e){
//    std::cout << "e: " <<  << std::endl;
}

LogEventWrap::~LogEventWrap()
{
    m_event->getLogger()->log(m_event->getLevel(),m_event);
}

std::stringstream& LogEventWrap::getSS(){
//    std::cout << "getfile: " << m_event->getLine() << std::endl;
//    std::cout << "**" << m_event->getSS().str() << std::endl;
    return m_event->getSS();
}
void Logger::log(LogLevel::Level level, const LogEvent::ptr event)
{
    if(level >= m_level)
    {
        auto self = shared_from_this();
        MutexType::Lock lock(m_mutex);
        if(!m_appenders.empty())
        {
            for(auto& i : m_appenders)
            {
                i->log(self, level, event);
            }
        } else {
            m_root->log(level, event);
        }

    }
}

void Logger::addAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    if(!appender->getFormatter()){
        MutexType::Lock ll(appender->m_mutex);
        appender->m_formatter = m_formatter;
//        appender->setFormatter(m_formatter);
    }
    m_appenders.push_back(appender);
}

void Logger::delAppender(LogAppender::ptr appender)
{
    MutexType::Lock lock(m_mutex);
    for(auto it = m_appenders.begin(); it != m_appenders.end(); it++)
    {
        if(*it == appender){
            m_appenders.erase(it);
            break;
        }
    }
}

void Logger::clearAppensers()
{
    MutexType::Lock lock(m_mutex);
    m_appenders.clear();
}

void Logger::debug(LogEvent::ptr event)
{
    log(LogLevel::DEBUG, event);
}

void Logger::info(LogEvent::ptr event)
{
    log(LogLevel::INFO, event);
}

void Logger::warn(LogEvent::ptr event)
{
    log(LogLevel::WARN, event);
}

void Logger::error(LogEvent::ptr event)
{
    log(LogLevel::ERROR, event);
}

void Logger::fatal(LogEvent::ptr event)
{
    log(LogLevel::FATAL, event);
}

class MessageFormatItem : public LogFormatter::FormatItem{
public:
    MessageFormatItem(const std::string& str = ""){}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getContent();
    }
};

class LevelFormatItem : public LogFormatter::FormatItem{
public:
    LevelFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << LogLevel::ToString(level);
    }
};

class ElapseFormatItem : public LogFormatter::FormatItem{
public:
    ElapseFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getElapse();
    }
};

class NameFormatItem : public LogFormatter::FormatItem{
public:
    NameFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLogger()->getName();
    }
};

class ThreadIdFormatItem : public LogFormatter::FormatItem{
public:
    ThreadIdFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getThreadId();
    }
};

class FiberIdFormatItem : public LogFormatter::FormatItem{
public:
    FiberIdFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getmFiberId();
    }
};

class TabFormatItem : public LogFormatter::FormatItem{
public:
    TabFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << "\t";
    }
};

class DateTimeFormatItem : public LogFormatter::FormatItem{
public:
//    DateTimeFormatItem(const std::string& format = "%Y:%m:%d %H:%M:%S"):m_format(format){

//    }
    DateTimeFormatItem(const std::string& format = "%Y-%m-%d %H:%M:%S"):m_format(format){
        if(m_format.empty())
        {
            m_format = "%Y-%m-%d %H:%M:%S";
        }
    }
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
//        os << event->getTime();
        struct tm tm;
        time_t time = event->getTime();
        localtime_r(&time, &tm);
        char buf[64];
        strftime(buf, sizeof(buf), m_format.c_str(), &tm);
        os << buf;
//        std::cout << buf;
    }
private:
    std::string m_format;
};

class FileNameFormatItem : public LogFormatter::FormatItem{
public:
    FileNameFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getFile();
    }
};

class LineFormatItem : public LogFormatter::FormatItem{
public:
    LineFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << event->getLine();
//        std::cout << event->getLine() << std::endl;
    }
};

class NewLineFormatItem : public LogFormatter::FormatItem{
public:
    NewLineFormatItem(const std::string& str = "") {}
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << std::endl;;
    }
};

class StringFormatItem : public LogFormatter::FormatItem{
public:
    StringFormatItem(const std::string& str):m_string(str)
    {

    }
    void format(std::shared_ptr<Logger> logger, std::ostream &os, LogLevel::Level level, LogEvent::ptr event) override {
        os << m_string;
    }
private:
    std::string m_string;
};

void LogAppender::setFormatter(LogFormatter::ptr val){
    MutexType::Lock lock(m_mutex);
    m_formatter = val;
    if(m_formatter) {
        m_hasFormatter = true;
    } else {
        m_hasFormatter = false;
    }
}

LogFormatter::ptr LogAppender::getFormatter()
{
    MutexType::Lock lock(m_mutex);
    return m_formatter;
}

FileLogAppender::FileLogAppender(const std::string &filename):m_filename(filename)
{
    reopen();
}

void FileLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    if(level >= m_level)
    {
        uint64_t now = time(0);
        if(now != m_lastTime){
            reopen();
            m_lastTime = now;
        }
        MutexType::Lock lock(m_mutex);
        m_filestream << m_formatter->format(logger, level, event);
    }
}

bool FileLogAppender::reopen()
{
    MutexType::Lock lock(m_mutex);
    if(m_filestream)
    {
        m_filestream.close();
    }
    m_filestream.open(m_filename, std::ios::app);
//    return FSUtil::OpenForWrite(m_filestream, m_filename, std::ios::app);
    return !!m_filestream;//???
}

std::string FileLogAppender::toYamlSreing(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "FileLogAppender";
    node["file"] = m_filename;
//    std::cout << "***  " << m_filename << "  ***" <<std::endl;
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter){
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

void StdoutLogAppender::log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
//    std::cout << "m_level:  " << level << " " << LogLevel::ToString(m_level) << std::endl;
    if(level >= m_level)
    {
        MutexType::Lock lock(m_mutex);
//        std::cout << "进入" <<std::endl;
        std::cout << m_formatter->format(logger, level, event);
//        std::string str =  m_formatter->format(logger, level, event);
//        std::cout << str << "***" <<std::endl;
    }
}

std::string StdoutLogAppender::toYamlSreing(){
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    node["type"] = "StdoutLogAppender";
    if(m_level != LogLevel::UNKNOW){
        node["level"] = LogLevel::ToString(m_level);
    }
    if(m_hasFormatter && m_formatter){ //
        node["formatter"] = m_formatter->getPattern();
    }
    std::stringstream ss;
    ss << node;
    return ss.str();
}

LogFormatter::LogFormatter(const std::string& pattern):m_pattern(pattern)
{
    init();
}

std::string LogFormatter::format(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)
{
    std::stringstream ss;
    for(auto& i : m_items)
    {
        i->format(logger, ss, level,  event);
//        ss << " ";
    }
    return ss.str();
}

// %xxx %xxx{xxx} %%
void LogFormatter::init()
{
    //str, format, type
    std::vector<std::tuple<std::string, std::string, int> > vec;
    std::string nstr;
    for(size_t i = 0; i < m_pattern.size(); ++i)
    {
        if(m_pattern[i] != '%')
        {
            nstr.append(1,m_pattern[i]);
            continue;
        }

        if( (i + 1) < m_pattern.size()){
            if(m_pattern[i + 1] == '%'){
                nstr.append(1, '%');
                continue;
            }
        }
        size_t n = i + 1;
        int fmt_status = 0;
        size_t fmt_begin = 0;

        std::string str;
        std::string fmt;

        while(n < m_pattern.size()){
            if(!fmt_status && (!isalpha(m_pattern[n]) && m_pattern[n] != '{' && m_pattern[n] != '}')){
                str = m_pattern.substr(i + 1, n - i - 1);
                break;
            }
            if(fmt_status == 0){
                if(m_pattern[n] == '{'){
                    str = m_pattern.substr(i + 1, n - i - 1);
                    fmt_status = 1;//解析格式
                    fmt_begin = n;
                    ++n;
                    continue;
                }
            }
            else if(fmt_status == 1){
                if(m_pattern[n] == '}')
                {
                    fmt = m_pattern.substr(fmt_begin + 1, n - fmt_begin - 1);
//                    std::cout << "#" << fmt << std::endl;
                    fmt_status = 0;
                    ++n;
                    break;
                }
            }
            ++n;
            if(n == m_pattern.size()){
                if(str.empty()){
                    str = m_pattern.substr(i + 1);
                }
            }
        }
        if(fmt_status == 0)
        {
            if(!nstr.empty()){
                vec.push_back(std::make_tuple(nstr, std::string(), 0));
                nstr.clear();
            }
            vec.push_back(std::make_tuple(str, fmt, 1));
            i = n - 1;
//            str = m_pattern.substr(i + 1 , n - i - 1);
////            std::cout<<i<<"  "<<str<<std::endl;
//            vec.push_back(std::make_tuple(str,fmt,1));
//            i = n - 1;
        }else if (fmt_status == 1){
            std::cout << "pattern parse error: "<< m_pattern << " - " << m_pattern.substr(i) << std::endl;
            m_error = true;
            vec.push_back(std::make_tuple("<<pattern_error>>",fmt,0));
        }
    }

    if(!nstr.empty()){
        vec.push_back(std::make_tuple(nstr, "", 0));
    }

    static std::map<std::string, std::function<FormatItem::ptr(const std::string& str)>> s_format_items = {
#define XX(str, C) \
        {#str, [](const std::string& fmt) { return FormatItem::ptr(new C(fmt));}}
        XX(m, MessageFormatItem),
        XX(p, LevelFormatItem),
        XX(r, ElapseFormatItem),
        XX(c, NameFormatItem),
        XX(t, ThreadIdFormatItem),
        XX(n, NewLineFormatItem),
        XX(d, DateTimeFormatItem),
        XX(f, FileNameFormatItem),
        XX(l, LineFormatItem),
        XX(T, TabFormatItem),
        XX(F, FiberIdFormatItem),
#undef XX
    };

    for(auto& i : vec) {
        if(std::get<2>(i) == 0) {
            m_items.push_back(FormatItem::ptr(new StringFormatItem(std::get<0>(i))));
        }else {
            auto it = s_format_items.find(std::get<0>(i));
            if(it == s_format_items.end()){
                m_items.push_back(FormatItem::ptr(new StringFormatItem("<<error_format %" + std::get<0>(i) + ">>")));
                m_error = true;
            } else {
                m_items.push_back(it->second(std::get<1>(i)));
            }
        }
//        std::cout << "(" << std::get<0>(i) << ") - (" << std::get<1>(i) << ") - (" << std::get<2>(i) << ")" << std::endl;
    }

//    std::cout << m_items.size() << std::endl;
}

    //%m -- 消息体
    //%p -- level
    //%r -- 启动后的时间
    //%c -- 日志名称
    //%t -- 线程id
    //%n -- 回车
    //%d -- 时间
    //%f -- 文件名
    //%l -- 行号

//const char* m_file = nullptr;    //文件名
//int32_t m_line = 0;              //行号
//uint32_t m_elapse;               //程序启动到现在的毫秒数
//uint32_t m_threadId = 0;         //线程id
//uint32_t m_fiberId = 0;          //协程id
//uint64_t m_time;                 //时间戳
//std::string m_content;//

LoggerManager::LoggerManager() {
    m_root.reset(new Logger);
    m_root->addAppender(LogAppender::ptr(new StdoutLogAppender));
    m_logger[m_root->m_name] = m_root;
    init();
}

struct LogAppenderDefine{
    int type = 0;//1 File, 2 Std
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;
    std::string file;

    bool operator ==(const LogAppenderDefine& oth) const {
        return type == oth.type
                && level == oth.level
                && formatter == oth.formatter
                && file == oth.file;
    }
};

struct LogDefine{
    std::string name;
    LogLevel::Level level = LogLevel::UNKNOW;
    std::string formatter;

    std::vector<LogAppenderDefine> appenders;

    bool operator ==(const LogDefine& oth) const {
        return name == oth.name
                && level == oth.level
                && formatter == oth.formatter
                && appenders == oth.appenders;
    }

    bool operator < (const LogDefine& oth) const {
        return name < oth.name;
    }
};

template<>
class LexicalCast<std::string, std::set<LogDefine> > {
public:
    std::set<LogDefine> operator ()(const std::string& v){
        YAML::Node node = YAML::Load(v);
        std::set<LogDefine> vec;

//        std::stringstream ss;
        for(size_t i = 0; i < node.size(); ++i){
            auto n = node[i];
            if(!n["name"].IsDefined()){
                std::cout << "logconfig error: name is null, " << n
                          << std::endl;
                continue;
            }

            LogDefine ld;
            ld.name = n["name"].as<std::string>();
            ld.level = LogLevel::FromString(n["level"].IsDefined() ? n["level"].as<std::string>() : "");
            if(n["formatter"].IsDefined()){
                ld.formatter = n["formatter"].as<std::string>();
            }

            if(n["appenders"].IsDefined()){
                for(size_t x = 0; x < n["appenders"].size(); ++x){
                    auto a = n["appenders"][x];
                    if(!a["type"].IsDefined()){
                        std::cout << "log config error: appender type is null, " << a
                                  <<std::endl;
                        continue;
                    }
                    std::string type = a["type"].as<std::string>();
                    LogAppenderDefine lad;
                    if(type == "FileLogAppender"){
                        lad.type = 1;
                        if(!a["file"].IsDefined()){
                            std::cout << "log config error: fileappender file is null, " << a
                                      <<std::endl;
                            continue;
                        }
//                        std::cout << "*****   " << a["file"].as<std::string>() << "    *****" <<std::endl;
                        lad.file = a["file"].as<std::string>();
                        if(a["formatter"].IsDefined()){
                            lad.formatter = a["formatter"].as<std::string>();
                        }
                    } else if(type == "StdoutLogAppender"){
                        lad.type = 2;
                    } else {
                        std::cout << "log config error: appender type is null, " << a
                                  <<std::endl;
                        continue;
                    }

                    ld.appenders.push_back(lad);
                }
            }
            vec.insert(ld);
        }
        return vec;
    }
};

template<>
class LexicalCast<std::set<LogDefine>,std::string> {
public:
    std::string operator ()(const std::set<LogDefine>& v){
        YAML::Node node;
        for(auto& i : v){
            YAML::Node n;
            n["name"] = i.name;
            if(i.level != LogLevel::UNKNOW){
                n["level"] = LogLevel::ToString(i.level);

            }

            if(!i.formatter.empty()){
                n["formatter"] = i.formatter;
            }

            for(auto& a : i.appenders){
                YAML::Node na;
                if(a.type == 1)
                {
                    na["type"] = "FileLogAppender";
                    na["file"] = a.file;
                } else if(a.type == 2){
                    na["type"] = "StdoutLogAppender";
                }
                if(a.level != LogLevel::UNKNOW){
                    na["level"] = LogLevel::ToString(a.level);
                }

                if(!a.formatter.empty()){
                    na["formatter"] = a.formatter;
                }
                n["appenders"].push_back(na);
            }
            node.push_back(n);
        }
//        for(auto& i : v){
//            node.push_back(YAML::Load(LexicalCast<T, std::string>()(i)));
//        }
        std::stringstream ss;
        ss << node;
        return ss.str();
    }
};

sylar::ConfigVar<std::set<LogDefine> >::ptr g_log_defines =
        sylar::Config::Lookup("logs", std::set<LogDefine>(), "logs config");

struct LogIniter{
    LogIniter() {
        g_log_defines->addListener([](const std::set<LogDefine>& old_value,
                                   const std::set<LogDefine>& new_value){

            std::cout << "****  cout hello system" << std::endl;
            SYLAR_LOG_INFO(SYLAR_LOG_ROOT()) << "on_logger_conf_changed";
            std::cout << "****  cout hello system" << std::endl;
            //新增
            for(auto& i : new_value){
                auto it = old_value.find(i);
                sylar::Logger::ptr logger;
                if(it == old_value.end()){
                    //新增logger
                    logger = SYLAR_LOG_NAME(i.name);
//                    logger.reset(new sylar::Logger(i.name));
                }
                else{
                    if(!(i == *it)){
                        //修改的logger
                        logger = SYLAR_LOG_NAME(i.name);
                    }
                }

                logger->setLevel(i.level);
                if(!i.formatter.empty()){
                    logger->setFormatter(i.formatter);
                }

                logger->clearAppensers();
                for(auto& a : i.appenders){
                    sylar::LogAppender::ptr ap;
                    if(a.type == 1){
                        ap.reset(new FileLogAppender(a.file));
                    }
                    else if(a.type == 2){
                        ap.reset(new StdoutLogAppender);
                    }
                    ap->setLevel(a.level);
                    if(!a.formatter.empty()){
                        LogFormatter::ptr fmt(new LogFormatter(a.formatter));
                        if(!fmt->isError()){
                            ap->setFormatter(fmt);
                        } else {
                            std::cout << "appender type = " << a.type << " formatter = " << a.formatter << " is invalid" << std::endl;
                        }
                    }
                    logger->addAppender(ap);
                }

            }

            //删除
            for(auto& i : old_value){
                auto it = new_value.find(i);
                if(it == new_value.end()){
                    //删除logger，并不是真的删除，当它没有（将文件关闭或者将日志等级设置很高
                    auto logger = SYLAR_LOG_NAME(i.name);
                    logger->setLevel((LogLevel::Level)100);
                    logger->clearAppensers();
                }
            }
        });
    }
};

static LogIniter __log_init;

void LoggerManager::init(){

}

std::string LoggerManager::toYamlString()
{
    MutexType::Lock lock(m_mutex);
    YAML::Node node;
    for(auto& i : m_logger) {
        node.push_back(YAML::Load(i.second->toYamlString()));
    }

    std::stringstream ss;
    ss << node;
    return ss.str();
}

Logger::ptr LoggerManager::getLogger(const std::string& name) {
    MutexType::Lock lock(m_mutex);
    auto it = m_logger.find(name);
    if(it != m_logger.end()){
        return it->second;
    }
    Logger::ptr logger(new Logger(name));
    logger->m_root = m_root;
    m_logger[name] = logger;
    return logger;
//    return it == m_logger.end() ? m_root : it->second;
}

}
