#ifndef __XLOG_H__
#define __XLOG_H__

#include "printf.h"


#define LOG_INFO(...) printf("[X-Hyper info] " __VA_ARGS__)
#define LOG_WARN(...) printf("[X-Hyper warn] " __VA_ARGS__)

#endif