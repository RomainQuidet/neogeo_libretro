#ifndef log_h
#define log_h

#import "libretro_core.h"

#define LOG_DEBUG RETRO_LOG_DEBUG
#define LOG_INFO  RETRO_LOG_INFO
#define LOG_ERROR RETRO_LOG_ERROR

#define LOG(x, ...) if (libretroCallbacks.log) libretroCallbacks.log(x, __VA_ARGS__)

#endif /* log_h */
