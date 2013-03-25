#ifndef _QUERY_STRING_

#define _QUERY_STRING_

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

char * query_string_implode(zval *arr, char *delim,int delim_len TSRMLS_DC);
char * quote_sql_str(char *src,size_t size,int like_escape);
uintptr_t escape_sql_str(char *dst,char *src,size_t size,int like_escape);
#endif;