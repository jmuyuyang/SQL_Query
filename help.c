#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_query.h"
#include "ext/standard/php_smart_str.h"
#include "help/query_string.h"

char * quote_sql_str(char *src,size_t size,int like_escape){
    int n = (int)escape_sql_str(NULL,src,size,like_escape);
    if(n == 0){
        return src;
    }else{
        char *dst;
        dst = (char *)emalloc(sizeof("''") - 1 + size + n);
        char *addr = dst;
        dst = (char *)escape_sql_str(dst,src,size,like_escape);
        *dst++ = '\0';
        return addr;
    }
}

uintptr_t escape_sql_str(char *dst,char *src,size_t size,int like_escape)
{
    unsigned int n;

    if (dst == NULL) {
        /* find the number of chars to be escaped */
        n = 0;
        while (size) {
            /* the highest bit of all the UTF-8 chars
             * is always 1 */
            if ((*src & 0x80) == 0) {
                switch (*src) {
                    case '\r':
                    case '\n':
                    case '\\':
                    case '\'':
                    case '"':
                    case '\032':
                        n++;
                        break;
                    case '%':
                    case '_':
                        if(like_escape) {
                            n++;
                        }
                        break;
                    default:
                        break;
                }
            }
            src++;
            size--;
        }
        return (uintptr_t) n;
    }

    while (size) {
        if ((*src & 0x80) == 0) {
            switch (*src) {
                case '\r':
                    *dst++ = '\\';
                    *dst++ = 'r';
                    break;

                case '\n':
                    *dst++ = '\\';
                    *dst++ = 'n';
                    break;

                case '\\':
                    *dst++ = '\\';
                    *dst++ = '\\';
                    break;

                case '\'':
                    *dst++ = '\\';
                    *dst++ = '\'';
                    break;

                case '"':
                    *dst++ = '\\';
                    *dst++ = '"';
                    break;

                case '\032':
                    *dst++ = '\\';
                    *dst++ = *src;
                    break;

                case '%':
                    if(like_escape){
                        *dst++ = '\\';
                        *dst++ = *src;
                    }
                    break;
                case '_':
                    if(like_escape){
                        *dst++ = '\\';
                        *dst++ = '_';
                    } 
                    break;

                default:
                    *dst++ = *src;
                    break;
            }
        } else {
            *dst++ = *src;
        }
        src++;
        size--;
    }
    return (uintptr_t) dst;
}

char *query_string_implode(zval *arr,char *delim, int delim_len TSRMLS_DC){
	zval         **tmp;
	HashPosition   pos;
	smart_str      implstr = {0};
	int            numelems, i = 0;
	zval tmp_val;
	int str_len;

	numelems = zend_hash_num_elements(Z_ARRVAL_P(arr));
	zend_hash_internal_pointer_reset_ex(Z_ARRVAL_P(arr), &pos);

	while (zend_hash_get_current_data_ex(Z_ARRVAL_P(arr), (void **) &tmp, &pos) == SUCCESS) {
		switch ((*tmp)->type) {
			case IS_STRING:{
                smart_str_appendl(&implstr, Z_STRVAL_PP(tmp),Z_STRLEN_PP(tmp));
                break;
            }
			case IS_LONG: {
				char stmp[MAX_LENGTH_OF_LONG + 1];
				str_len = slprintf(stmp, sizeof(stmp), "%ld", Z_LVAL_PP(tmp));
				smart_str_appendl(&implstr, stmp, str_len);
			}
				break;

			case IS_BOOL:
				if (Z_LVAL_PP(tmp) == 1) {
					smart_str_appendl(&implstr, "1", sizeof("1")-1);
				}
				break;

			case IS_NULL:
				break;

			case IS_DOUBLE: {
				char *stmp;
				str_len = spprintf(&stmp, 0, "%.*G", (int) EG(precision), Z_DVAL_PP(tmp));
				smart_str_appendl(&implstr, stmp, str_len);
				efree(stmp);
			}
				break;

			case IS_OBJECT: {
				int copy;
				zval expr;
				zend_make_printable_zval(*tmp, &expr, &copy);
				smart_str_appendl(&implstr, Z_STRVAL(expr), Z_STRLEN(expr));
				if (copy) {
					zval_dtor(&expr);
				}
			}
				break;

			default:
				tmp_val = **tmp;
				zval_copy_ctor(&tmp_val);
				convert_to_string(&tmp_val);
				smart_str_appendl(&implstr, Z_STRVAL(tmp_val), Z_STRLEN(tmp_val));
				zval_dtor(&tmp_val);
				break;

		}

		if (++i != numelems) {
			smart_str_appendl(&implstr, delim, delim_len);
		}
		zend_hash_move_forward_ex(Z_ARRVAL_P(arr), &pos);
	}
	smart_str_0(&implstr);
	if (implstr.len) {
		char *data = (char *) emalloc(implstr.len + 1);
		memcpy(data,implstr.c,implstr.len+1);
        return data;
	} else {
		return NULL;
	}
}
