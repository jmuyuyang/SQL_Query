/*
  +----------------------------------------------------------------------+
  | PHP Version 5                                                        |
  +----------------------------------------------------------------------+
  | Copyright (c) 1997-2012 The PHP Group                                |
  +----------------------------------------------------------------------+
  | This source file is subject to version 3.01 of the PHP license,      |
  | that is bundled with this package in the file LICENSE, and is        |
  | available through the world-wide-web at the following url:           |
  | http://www.php.net/license/3_01.txt                                  |
  | If you did not receive a copy of the PHP license and are unable to   |
  | obtain it through the world-wide-web, please send a note to          |
  | license@php.net so we can mail you a copy immediately.               |
  +----------------------------------------------------------------------+
  | Author:                                                              |
  +----------------------------------------------------------------------+
*/

/* $Id$ */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_query.h"
#include "ext/standard/php_smart_str.h"
#include "help/query_string.h"

/* If you declare any globals in php_query.h uncomment this:
ZEND_DECLARE_MODULE_GLOBALS(query)
*/

/* True global resources - no need for thread safety here */
static int le_query;
zend_class_entry *query_ce;
/* {{{ query_functions[]
 *
 * Every user visible function must have an entry in query_functions[].
 */
 
int parse_array_condition(char *key,zval *arr,char **conval){
    zval **oper_tmp;
    zval **val_tmp;
    HashTable *h = Z_ARRVAL_P(arr);
    zend_hash_index_find(h,0,(void **)&oper_tmp);
    if(Z_TYPE_PP(oper_tmp) == IS_STRING){
        char *oper = Z_STRVAL_PP(oper_tmp);
        zend_hash_index_find(h,1,(void **)&val_tmp);
        if(strncmp(oper,"<",1) == 0){
        	return spprintf(conval,0,"%s %s %ld",key,oper,Z_LVAL_PP(val_tmp));
        }

        if(strncmp(oper,">",1) == 0){
        	return spprintf(conval,0,"%s %s %ld",key,oper,Z_LVAL_PP(val_tmp));
        }

        if(strcmp(oper,"+") == 0){
        	return spprintf(conval,0,"%s %s %ld",key,oper,Z_LVAL_PP(val_tmp));
        }

        if(strcmp(oper,"like") == 0){
        	return spprintf(conval,0,"%s %s '%s'",key,oper,quote_sql_str(Z_STRVAL_PP(val_tmp),Z_STRLEN_PP(val_tmp),0));
        }

        if(strcmp(oper,"not like") == 0){
        	return spprintf(conval,0,"%s %s '%s'",key,oper,quote_sql_str(Z_STRVAL_PP(val_tmp),Z_STRLEN_PP(val_tmp),0));
        }
    }
   	return spprintf(conval,0,"%s in (%s)",key,query_string_implode(arr,",",1 TSRMLS_CC));
 }

 void parse_condition(zval *condition,zval *condition_arr){
    zval **data;
    char *key;
    ulong ikey;
    char *conval;
    int conval_len;
    int i = 0;
    HashTable *h = Z_ARRVAL_P(condition);
    zend_hash_internal_pointer_reset(h);
    int count = zend_hash_num_elements(h);
    for(i=0;i<count;i++){
        zend_hash_get_current_key(h,&key,&ikey,0);
        zend_hash_get_current_data(h,(void **)&data);
        switch(Z_TYPE_PP(data)){
            case IS_ARRAY:{
		        conval_len = parse_array_condition(key,*data,&conval);
		        add_index_stringl(condition_arr,i,conval,conval_len,0);
            };break;
            case IS_LONG:{
                conval_len = spprintf(&conval,0,"%s = %ld",key,Z_LVAL_PP(data));
                add_index_stringl(condition_arr,i,conval,conval_len,0);
            };break;
            case IS_STRING:{
            	if(strncmp(Z_STRVAL_PP(data),":",1) == 0){
            		conval_len = spprintf(&conval,0,"%s = %s",key,Z_STRVAL_PP(data));
            	}else{
            		conval_len = spprintf(&conval,0,"%s = '%s'",key,quote_sql_str(Z_STRVAL_PP(data),Z_STRLEN_PP(data),1));  
                }
                add_index_stringl(condition_arr,i,conval,conval_len,0);
            };break;
        }
        zend_hash_move_forward(h);
    }
}

ZEND_METHOD(SqlQuery,setTable){
	zval *self = getThis();
	zval *table;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &table) == FAILURE){
		RETURN_NULL();
	}
	if(Z_TYPE_P(table) == IS_STRING){
		zend_update_property(Z_OBJCE_P(self),self,"table",sizeof("table")-1,table TSRMLS_CC);
	}
	RETURN_ZVAL(self, 1, 0);
}

ZEND_METHOD(SqlQuery,escape){
	char *str;
	char *escape_str;
	long like_escape = 1;
	int str_len;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"s|l",&str,&str_len,&like_escape) == FAILURE){
		RETURN_NULL();
	}
	int escape_str_len = spprintf(&escape_str,0,"'%s'",quote_sql_str(str,str_len,(int)like_escape));
	RETURN_STRINGL(escape_str,escape_str_len,0);
}

ZEND_METHOD(SqlQuery,field){
	zval *fields;
	zval *self = getThis();
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "a", &fields) == FAILURE){
		RETURN_NULL();
	}
	zend_update_property_string(Z_OBJCE_P(self),self,ZEND_STRL("field"),query_string_implode(fields,",",1 TSRMLS_CC));
	RETURN_ZVAL(self, 1, 0);
}

ZEND_METHOD(SqlQuery,where){
	zval *condition;
	zval *self = getThis();
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z", &condition) == FAILURE){
		RETURN_NULL();
	}
	if(Z_TYPE_P(condition) == IS_STRING){
		zend_update_property(Z_OBJCE_P(self),self,"where",sizeof("where")-1,condition TSRMLS_CC);
	}else if(Z_TYPE_P(condition) == IS_ARRAY){
		zval *condition_arr;
		zval *result;
		MAKE_STD_ZVAL(condition_arr);
		array_init(condition_arr);
		parse_condition(condition,condition_arr);
		MAKE_STD_ZVAL(result);
		char *result_str = query_string_implode(condition_arr," and ",5 TSRMLS_CC);
		if(result_str){
			ZVAL_STRING(result,result_str,0);
			zend_update_property(Z_OBJCE_P(self),self,"where",sizeof("where")-1,result TSRMLS_CC);
		}
		zval_dtor(condition_arr);
	}
	RETURN_ZVAL(self, 1, 0);
}

ZEND_METHOD(SqlQuery,limit){
	long limit;
	long offset = 0;
	zval *self = getThis();
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "l|l", &limit,&offset) == FAILURE){
		RETURN_NULL();
	}
	if(limit > 0){
		zend_update_property_long(Z_OBJCE_P(self),self,ZEND_STRL("limit"),limit TSRMLS_CC);
	}
	if(offset > 0){
		zend_update_property_long(Z_OBJCE_P(self),self,ZEND_STRL("offset"),offset TSRMLS_CC);
	}
	RETURN_ZVAL(self, 1, 0);
}

ZEND_METHOD(SqlQuery,order){
	zval *order;
	zval *self = getThis();
	smart_str orderstr = {0};
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"a",&order) == FAILURE){
		RETURN_NULL();
	}
	char *field;
	char *order_result;
	ulong idx;
	int i,order_len;
	zval **val;
	HashTable *h = Z_ARRVAL_P(order);
    zend_hash_internal_pointer_reset(h);
    int count = zend_hash_num_elements(h);
    for(i=0;i<count;i++){
		zend_hash_get_current_key(h,&field,&idx,0);
       	zend_hash_get_current_data(h,(void **)&val);
       	if(Z_TYPE_PP(val) == IS_STRING){
        	order_len = spprintf(&order_result,0,"%s %s",field,Z_STRVAL_PP(val));
        	smart_str_appendl(&orderstr,order_result,order_len);
        	if(i != count-1){
        		smart_str_appendl(&orderstr,", ",2);
        	}
       	}
 		zend_hash_move_forward(h);
	}
	smart_str_0(&orderstr);
	zend_update_property_stringl(Z_OBJCE_P(self),self,ZEND_STRL("order"),orderstr.c,orderstr.len TSRMLS_CC);
	RETURN_ZVAL(self, 1, 0);
}

ZEND_METHOD(SqlQuery,select){
	zval *self = getThis();
	zval *table;
	zval *field;
	zval *where;
	zval *order;
	zval *limit;
	zval *offset;
	smart_str sqlstr = {0};
	char *sql;
	int sql_len;
	table = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("table"), 0 TSRMLS_CC);
	field = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("field"), 0 TSRMLS_CC);   
	where = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("where"), 0 TSRMLS_CC);  
	limit = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("limit"), 0 TSRMLS_CC);  
	offset = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("offset"), 0 TSRMLS_CC);  
	order = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("order"), 0 TSRMLS_CC);   
	if(Z_TYPE_P(table) == IS_STRING){
		if(Z_TYPE_P(where) == IS_STRING){
			char *where_str;
			int where_len = spprintf(&where_str,0," where %s",Z_STRVAL_P(where));
			smart_str_appendl(&sqlstr,where_str,where_len);
		}
		if(Z_TYPE_P(order) == IS_STRING){
			char *order_str;
			int order_len = spprintf(&order_str,0," order by %s",Z_STRVAL_P(order));
			smart_str_appendl(&sqlstr,order_str,order_len);
		}
		if(Z_LVAL_P(limit) > 0){
			char *limit_str;
			int limit_len;
			if(Z_LVAL_P(offset) > 0){
				limit_len = spprintf(&limit_str,0," limit %ld %ld",Z_LVAL_P(limit),Z_LVAL_P(offset));
			}else{
				limit_len = spprintf(&limit_str,0," limit %ld",Z_LVAL_P(limit));
			}
			smart_str_appendl(&sqlstr,limit_str,limit_len);
		}
		smart_str_0(&sqlstr);
		if(sqlstr.len){
			sql_len = spprintf(&sql,0,"select %s from %s%s",Z_STRVAL_P(field),Z_STRVAL_P(table),sqlstr.c);
		}else{
			sql_len = spprintf(&sql,0,"select %s from %s",Z_STRVAL_P(field),Z_STRVAL_P(table));
		}
		RETURN_STRINGL(sql,sql_len,0);
	}
}

ZEND_METHOD(SqlQuery,insert){
	char *str;
	int str_len,numelems;
	char *result;
	zval *insert_data;
	zval *self = getThis();
	smart_str fieldstr = {0};
	smart_str valstr = {0};
	int i = 0;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"a",&insert_data) == FAILURE){
		RETURN_NULL();
	}
	if(Z_TYPE_P(insert_data) == IS_ARRAY){
		HashTable *h = Z_ARRVAL_P(insert_data);
		HashPosition  pos;
		zval **tmp;
		char *key;
		int ikey;
		ulong num_key;
		numelems = zend_hash_num_elements(h);
		zend_hash_internal_pointer_reset_ex(h, &pos);
		while (zend_hash_get_current_data_ex(h, (void **) &tmp, &pos) == SUCCESS) {
			zend_hash_get_current_key_ex(h,&key,&ikey,&num_key,0,&pos);
			smart_str_appendl(&fieldstr,key,ikey-1);
			switch ((*tmp)->type) {
				case IS_STRING:{
					str_len = spprintf(&str,0,"'%s'",quote_sql_str(Z_STRVAL_PP(tmp), Z_STRLEN_PP(tmp),1));
					smart_str_appendl(&valstr,str,str_len);
				}break;
				case IS_LONG:{
					char stmp[MAX_LENGTH_OF_LONG + 1];
					str_len = slprintf(stmp, sizeof(stmp), "%ld", Z_LVAL_PP(tmp));
					smart_str_appendl(&valstr, stmp, str_len);
				}break;
			}
			if (++i != numelems) {
				smart_str_appendl(&fieldstr, ", ",2);
				smart_str_appendl(&valstr, ", ",2);
			}
			zend_hash_move_forward_ex(h, &pos);
		}
		smart_str_0(&fieldstr);
		smart_str_0(&valstr);
		zval *table;
		table = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("table"), 0 TSRMLS_CC);  
		int result_len = spprintf(&result,0,"insert into %s (%s) values (%s)",Z_STRVAL_P(table),fieldstr.c,valstr.c);
		RETURN_STRINGL(result,result_len,0);
	}
}

ZEND_METHOD(SqlQuery,update){
	zval *self = getThis();
	zval *update_condition;
	zval *select_condition;
	zval *update_condition_arr;
	zval *select_condition_arr;
	zval *table;
	char *sql;
	char *select;
	char *update;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "az", &update_condition,&select_condition) == FAILURE){
		RETURN_NULL();
	}
	MAKE_STD_ZVAL(update_condition_arr);
	array_init(update_condition_arr);
	parse_condition(update_condition,update_condition_arr);
	update = query_string_implode(update_condition_arr,",",1 TSRMLS_CC);
	if(Z_TYPE_P(select_condition) == IS_STRING){
		select = Z_STRVAL_P(select_condition);
	}else if(Z_TYPE_P(select_condition) == IS_ARRAY){
		MAKE_STD_ZVAL(select_condition_arr);
		array_init(select_condition_arr);
		parse_condition(select_condition,select_condition_arr);
		select = query_string_implode(select_condition_arr," and ",5 TSRMLS_CC);
	}
	table = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("table"), 0 TSRMLS_CC);  
	int sql_len = spprintf(&sql,0,"update %s set %s where %s",Z_STRVAL_P(table),update,select);
	zval_dtor(update_condition_arr);
	zval_dtor(select_condition_arr);
	RETURN_STRINGL(sql,sql_len,0);
}

ZEND_METHOD(SqlQuery,delete){
	zval *condition;
	zval *condition_arr;
	zval *table;
	zval *self = getThis();
	char *where;
	char *sql;
	if(zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC,"z",&condition) == FAILURE){
		RETURN_NULL();
	}
	if(Z_TYPE_P(condition) == IS_STRING){
		where = Z_STRVAL_P(condition);
	}else if(Z_TYPE_P(condition) == IS_ARRAY){
		MAKE_STD_ZVAL(condition_arr);
		array_init(condition_arr);
		parse_condition(condition,condition_arr);
		where = query_string_implode(condition_arr," and ",5 TSRMLS_CC);
	}
	table = zend_read_property(Z_OBJCE_P(self), self, ZEND_STRL("table"), 0 TSRMLS_CC);  
	int sql_len = spprintf(&sql,0,"delete from %s where %s",Z_STRVAL_P(table),where);
	zval_dtor(condition_arr);
	RETURN_STRINGL(sql,sql_len,0); 
}


static zend_function_entry SqlQuery_method[]={
	ZEND_ME(SqlQuery,setTable,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,escape,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,field,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,where,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,limit,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,order,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,select,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,insert,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,update,NULL,ZEND_ACC_PUBLIC)
	ZEND_ME(SqlQuery,delete,NULL,ZEND_ACC_PUBLIC)
	{NULL,NULL,NULL}
};

const zend_function_entry query_functions[] = {
	PHP_FE(confirm_query_compiled,	NULL)		/* For testing, remove later. */
	{NULL,NULL,NULL}	/* Must be the last line in query_functions[] */
};
/* }}} */

/* {{{ query_module_entry
 */
zend_module_entry query_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"query",
	query_functions,
	PHP_MINIT(query),
	PHP_MSHUTDOWN(query),
	PHP_RINIT(query),		/* Replace with NULL if there's nothing to do at request start */
	PHP_RSHUTDOWN(query),	/* Replace with NULL if there's nothing to do at request end */
	PHP_MINFO(query),
#if ZEND_MODULE_API_NO >= 20010901
	"0.1", /* Replace with version number for your extension */
#endif
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_QUERY
ZEND_GET_MODULE(query)
#endif

/* {{{ PHP_INI
 */
/* Remove comments and fill if you need to have entries in php.ini
PHP_INI_BEGIN()
    STD_PHP_INI_ENTRY("query.global_value",      "42", PHP_INI_ALL, OnUpdateLong, global_value, zend_query_globals, query_globals)
    STD_PHP_INI_ENTRY("query.global_string", "foobar", PHP_INI_ALL, OnUpdateString, global_string, zend_query_globals, query_globals)
PHP_INI_END()
*/
/* }}} */

/* {{{ php_query_init_globals
 */
/* Uncomment this function if you have INI entries
static void php_query_init_globals(zend_query_globals *query_globals)
{
	query_globals->global_value = 0;
	query_globals->global_string = NULL;
}
*/
/* }}} */

/* {{{ PHP_MINIT_FUNCTION
 */
PHP_MINIT_FUNCTION(query)
{
	/* If you have INI entries, uncomment these lines 
	REGISTER_INI_ENTRIES();
	*/
	zend_class_entry ce;
	INIT_CLASS_ENTRY(ce,"SqlQuery",SqlQuery_method);
	query_ce = zend_register_internal_class(&ce TSRMLS_CC);
	zend_declare_property_stringl(query_ce,"field",sizeof("field")-1,"*",1,ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(query_ce,"order",sizeof("order")-1,ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_long(query_ce,"limit",sizeof("limit")-1,0,ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_long(query_ce,"offset",sizeof("offset")-1,0,ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(query_ce,"where",sizeof("where")-1,ZEND_ACC_PUBLIC TSRMLS_CC);
	zend_declare_property_null(query_ce,"table",sizeof("table")-1,ZEND_ACC_PUBLIC TSRMLS_CC);
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MSHUTDOWN_FUNCTION
 */
PHP_MSHUTDOWN_FUNCTION(query)
{
	/* uncomment this line if you have INI entries
	UNREGISTER_INI_ENTRIES();
	*/
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request start */
/* {{{ PHP_RINIT_FUNCTION
 */
PHP_RINIT_FUNCTION(query)
{
	return SUCCESS;
}
/* }}} */

/* Remove if there's nothing to do at request end */
/* {{{ PHP_RSHUTDOWN_FUNCTION
 */
PHP_RSHUTDOWN_FUNCTION(query)
{
	return SUCCESS;
}
/* }}} */

/* {{{ PHP_MINFO_FUNCTION
 */
PHP_MINFO_FUNCTION(query)
{
	php_info_print_table_start();
	php_info_print_table_header(2, "query support", "enabled");
	php_info_print_table_end();

	/* Remove comments if you have entries in php.ini
	DISPLAY_INI_ENTRIES();
	*/
}
/* }}} */


/* Remove the following function when you have succesfully modified config.m4
   so that your module can be compiled into PHP, it exists only for testing
   purposes. */

/* Every user-visible function in PHP should document itself in the source */
/* {{{ proto string confirm_query_compiled(string arg)
   Return a string to confirm that the module is compiled in */
PHP_FUNCTION(confirm_query_compiled)
{
	char *arg = NULL;
	int arg_len, len;
	char *strg;

	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &arg, &arg_len) == FAILURE) {
		return;
	}

	len = spprintf(&strg, 0, "Congratulations! You have successfully modified ext/%.78s/config.m4. Module %.78s is now compiled into PHP.", "query", arg);
	RETURN_STRINGL(strg, len, 0);
}
/* }}} */
/* The previous line is meant for vim and emacs, so it can correctly fold and 
   unfold functions in source code. See the corresponding marks just before 
   function definition, where the functions purpose is also documented. Please 
   follow this convention for the convenience of others editing your code.
*/


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
