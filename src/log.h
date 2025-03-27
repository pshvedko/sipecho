/*
 * log.h
 *
 *  Created on: Sep 24, 2013
 *      Author: shved
 */

#ifndef LOG_H_
#define LOG_H_

#include <libgen.h>
#include <signal.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define COLOR(a,c)	"\e["#a";"#c"m"

#define FD_NORMAL	COLOR(0, 00)

#define FD_BLACK	COLOR(0, 30)
#define FD_RED 		COLOR(0, 31)
#define FD_GREEN 	COLOR(0, 32)
#define FD_YELLOW 	COLOR(0, 33)
#define FD_BLUE 	COLOR(0, 34)
#define FD_MAGENTA 	COLOR(0, 35)
#define FD_CYAN 	COLOR(0, 36)
#define FD_WHITE 	COLOR(0, 37)

#define FL_BLACK	COLOR(1, 30)
#define FL_RED 		COLOR(1, 31)
#define FL_GREEN 	COLOR(1, 32)
#define FL_YELLOW 	COLOR(1, 33)
#define FL_BLUE 	COLOR(1, 34)
#define FL_MAGENTA 	COLOR(1, 35)
#define FL_CYAN 	COLOR(1, 36)
#define FL_WHITE 	COLOR(1, 37)

#define BD_BLACK 	COLOR(0, 40)
#define BD_RED 		COLOR(0, 41)
#define BD_GREEN 	COLOR(0, 42)
#define BD_YELLOW 	COLOR(0, 43)
#define BD_BLUE 	COLOR(0, 44)
#define BD_MAGENTA 	COLOR(0, 45)
#define BD_CYAN 	COLOR(0, 46)
#define BD_WHITE 	COLOR(0, 47)

#define log_debug( f, ... ) 	logit( LOG_DEBUG,	"[D] " f, ##__VA_ARGS__ )
#define log_info( f, ... ) 		logit( LOG_INFO,	"[I] " f, ##__VA_ARGS__ )
#define log_notice( f, ... ) 	logit( LOG_NOTICE,	"[N] " f, ##__VA_ARGS__ )
#define log_warning( f, ... ) 	logit( LOG_WARNING,	"[W] " f, ##__VA_ARGS__ )
#define log_error( f, ... ) 	logit( LOG_ERR, 	"[E] " f, ##__VA_ARGS__ )
#define log_crit( f, ... ) 		logit( LOG_CRIT,	"[C] " f, ##__VA_ARGS__ )
#define log_alert( f, ... ) 	logit( LOG_ALERT,	"[A] " f, ##__VA_ARGS__ )
#define log_fatal( f, ... ) 	logit( LOG_EMERG,	"[F] " f, ##__VA_ARGS__ )

#define log_dump( b, l )		hexit( LOG_DEBUG, b, l, 16 )

#define log_open( n, p, f )		openlog( n, p, f);
#define log_level( l )			setlogmask( LOG_UPTO( l ) )
#define log_increase()			setlogmask( ( ( setlogmask(0) << 1 ) + 1 ) )
#define log_decrease()			setlogmask( ( ( setlogmask(0) >> 1 ) ) )
#define log_close()				closelog()

int logit(int, const char *, ...);
int hexit(int, const char *, unsigned, int);

#endif /* LOG_H_ */
