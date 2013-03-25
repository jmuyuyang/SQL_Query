dnl $Id$
dnl config.m4 for extension query

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

dnl PHP_ARG_WITH(query, for query support,
dnl Make sure that the comment is aligned:
dnl [  --with-query             Include query support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(query, whether to enable query support,
Make sure that the comment is aligned:
[  --enable-query           Enable query support])

if test "$PHP_QUERY" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-query -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/query.h"  # you most likely want to change this
  dnl if test -r $PHP_QUERY/$SEARCH_FOR; then # path given as parameter
  dnl   QUERY_DIR=$PHP_QUERY
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for query files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       QUERY_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$QUERY_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the query distribution])
  dnl fi

  dnl # --with-query -> add include path
  dnl PHP_ADD_INCLUDE($QUERY_DIR/include)

  dnl # --with-query -> check for lib and symbol presence
  dnl LIBNAME=query # you may want to change this
  dnl LIBSYMBOL=query # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $QUERY_DIR/lib, QUERY_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_QUERYLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong query lib version or lib not found])
  dnl ],[
  dnl   -L$QUERY_DIR/lib -lm
  dnl ])
  dnl
  dnl PHP_SUBST(QUERY_SHARED_LIBADD)

  PHP_NEW_EXTENSION(query, query.c help.c, $ext_shared)
fi
