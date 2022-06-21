#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

// #include<mutex>
// static std::mutex logger_mutex;

void LoggerUI();
void Log(const char* fmt, ...);// IM_FMTARGS(2);


#endif