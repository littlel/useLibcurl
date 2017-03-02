/**
 * FileName:	outctrl.h
 * Author:		chsMiao
 * Description: 实现简单的日志输出功能
 * 
 * Details: 
 * 1. 有两种日志输出方式选择
 *    a) 自定义的格式输出（默认）
 *		 相比 int fprintf ( FILE * stream, const char * format, ... ); 
 *			  增加了前置时间戳 和后置的文件名及行号
 *		 如： 2017-02-13 09:40:54 | hello world! ---[main.cpp:5]
 *    b) log4cplus的日志输出
 *		 这里采用静态链接的方式，注意字符集为Unicode（可根据情况调整）
 *		 这里默认使用INFO级别并使用脚本配置进行输出控制
 *		 当使用此种方式，要注意log4cplus的include和lib目录的引入
 * 
 * 2. 所涉及到的宏
 *    USE_LOG4CPLUS   定义了此宏，表示采用log4cplus的日志输出
 *	  MAX_MSG_LENGTH  采用log4cplus的日志输出时的消息体最大的长度
 *					  相当于 【"%m"，输出原始信息】的最大长度
 *	  CFG_NAME		  log4cplus日志的配置文件,默认"RealUse.cfg"
 *    LOG_SHUTDOWN    关闭日志输出功能
 *    CLOG			  实现的最终用于日志输出的宏	
 *
 *    注意：前四个宏的定义要在引入outctrl.h头文件之前才有用，
 *         可以放在VS工程属性的"预处理器定义"中
 *
 * 3. 使用方法
 *	  引入该头文件outctrl.h后，使用CLOG进行输出即可
 *	  如：
			#define USE_LOG4CPLUS
			#include "outctrl.h"
			int main(int argc, char **argv)
			{
				CLOG("hello world!");
				CLOG("%s  %d %.2f", "hello", 123, 3.1415926);
				return 0;
			}
 *
 */

#ifndef __CTRLOUT_H_
#define __CTRLOUT_H_

#include <string>
#include <sstream>
#include <ctime>
#include <cstdarg>   //让函数接受可变参数

#ifdef USE_LOG4CPLUS

#ifdef _DEBUG
#pragma comment(lib, "log4cplusSUD.lib")
#else
#pragma comment(lib, "log4cplusSU.lib")
#endif

#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/loggingmacros.h>

using namespace log4cplus;

#ifndef CFG_NAME
#define CFG_NAME "RealUse.cfg"
#endif

#ifndef MAX_MSG_LENGTH
#define MAX_MSG_LENGTH	2048
#endif

#endif // USE_LOG4CPLUS

using std::string;
using std::stringstream;


#ifdef LOG_SHUTDOWN
	#define CLOG(fmt, ...)
#else
	#ifndef USE_LOG4CPLUS

	#define CLOG(format, ...)						 \
	do {											 \
		char time_buff[64];							 \
		auto now = time(nullptr);					 \
		strftime(time_buff, sizeof(time_buff),		 \
				"%Y-%m-%d %H:%M:%S | ",				 \
				localtime(&now));					 \
		fprintf(stdout, "%s", time_buff);			 \
		fprintf(stdout, format, ##__VA_ARGS__);      \
		fprintf(stdout, " ---["__FILE__":%d] \n",    \
				__LINE__);							 \
	} while (0)										 \

	#else

	#define CLOG(fmt, ...)							 \
	do{												 \
	char output_buff[MAX_MSG_LENGTH];				 \
	sprintf(output_buff, fmt, ##__VA_ARGS__);		 \
	PropertyConfigurator::doConfigure(				 \
						  LOG4CPLUS_TEXT(CFG_NAME)); \
	LOG4CPLUS_INFO(Logger::getRoot(), output_buff);  \
	} while(0)										 \

	#endif	// USE_LOG4CPLUS
#endif		// LOG_SHUTDOWN

#endif  	// __CTRLOUT_H_