#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <glib.h>

void qa_time_to_gmt_str(char *str_gmt_time, time_t *time_in);


void qa_time_to_gmt_str(char *str_gmt_time, time_t *time_in)
{
	struct tm *strtime;
	strtime = gmtime(time_in);
	
	sprintf(str_gmt_time, "%d/%d/%d %d:%d:%d", 
		strtime->tm_mon, strtime->tm_mday, strtime->tm_year+1900,
		strtime->tm_hour, strtime->tm_min, strtime->tm_sec);

	return;
}

