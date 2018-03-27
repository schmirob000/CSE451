#ifndef JOS_INC_TIME_H
#define JOS_INC_TIME_H

#include <inc/types.h>

#define NANOSECONDS_PER_MILLISECOND	UINT64_C(1000000)
#define NANOSECONDS_PER_SECOND		UINT64_C(1000000000)

typedef uint64_t seconds_t;
typedef uint64_t nanoseconds_t;

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
};

#endif	// !JOS_INC_TIME_H
