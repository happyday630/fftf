/*test_common.c*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include <glib.h>
#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <glib-object.h>
#include <gobject/gvaluecollector.h>
/*----------------------------qaDbusPeerServerNewConnect----------------------------*/
/*
@serverAddr: 
@connHandle: 
*/
DBusGConnection*  
qaDbusPeerServerNewConnect(const char* serverAddr)
{
	GError* error = NULL;
	DBusGConnection* connHandle = NULL;
	
	if(NULL==serverAddr) {
		printf("ERROR: input parameter serverAddr is NULL.\n");
		return NULL;
	};
	
	connHandle = dbus_g_connection_open (serverAddr, &error);
	if(NULL==connHandle) {
		g_error ("FAIL: cannot get connection: %s", error->message);
		return NULL;
	};
	
	printf("INFO: appman server connection handle is: 0x%08x\n", connHandle);
	return connHandle;
}

/*----------------------------qaIsDbusConnectionConnected----------------------------*/
/*
@gConnHandle:
*/
gboolean 
qaIsDbusConnectionConnected(DBusGConnection* gConnHandle)
{
	DBusConnection* busconn = NULL;
	
	if(NULL==gConnHandle) {
		printf("ERROR: input gconnection handle is NULL.\n");
		return FALSE;
	};

	busconn = dbus_g_connection_get_connection (gConnHandle);
	if(NULL==busconn) {
		printf("FAIL: can't get dbus connection handle from dbus g connection handle.\n");
		return FALSE;
	};

	if(TRUE==dbus_connection_get_is_connected (busconn)) {
		return TRUE;
	}
	else {
		return FALSE;
	};
}

/*----------------------------qaIsDbusConnectionAnonymous----------------------------*/
/*
@gConnHandle:
*/
gboolean 
qaIsDbusConnectionAnonymous(DBusGConnection* gConnHandle)
{
	DBusConnection* busconn = NULL;
	
	if(NULL==gConnHandle) {
		printf("ERROR: input gconnection handle is NULL.\n");
		return FALSE;
	};

	busconn = dbus_g_connection_get_connection (gConnHandle);
	if(NULL==busconn) {
		printf("FAIL: can't get dbus connection handle from dbus g connection handle.\n");
		return FALSE;
	};

	if(TRUE==dbus_connection_get_is_anonymous (busconn)) {
		return TRUE;
	}
	else {
		return FALSE;
	};
}

/*----------------------------qaDbusConnectionGetUnixUser----------------------------*/
/*
@gConnHandle:
@uid:
*/
gboolean 
qaDbusConnectionGetUnixUser(DBusGConnection* gConnHandle, unsigned long* uid)
{
	DBusConnection* busconn = NULL;
	
	if(NULL==gConnHandle) {
		printf("ERROR: input gconnection handle is NULL.\n");
		uid = NULL;
		return FALSE;
	};

	if(NULL==uid) {
		printf("ERROR: input parameter uid is NULL.\n");
		return FALSE;
	};

	busconn = dbus_g_connection_get_connection (gConnHandle);
	if(NULL==busconn) {
		printf("FAIL: can't get dbus connection handle from dbus g connection handle.\n");
		uid = NULL;
		return FALSE;
	};

	if(TRUE==dbus_connection_get_unix_user (busconn, uid)) {
		return TRUE;
	}
	else {
		return FALSE;
	};
}

/*----------------------------qaDbusConnectionGetMaxMsgSize----------------------------*/
/*
@gConnHandle:
*/
long
qaDbusConnectionGetMaxMsgSize(DBusGConnection* gConnHandle)
{
	DBusConnection* busconn = NULL;
	
	if(NULL==gConnHandle) {
		printf("ERROR: input gconnection handle is NULL.\n");
		return FALSE;
	};

	busconn = dbus_g_connection_get_connection (gConnHandle);
	if(NULL==busconn) {
		printf("FAIL: can't get dbus connection handle from dbus g connection handle.\n");
		return FALSE;
	};

	return dbus_connection_get_max_message_size (busconn);
}

/*----------------------------qaDbusConnectionGetProcessId----------------------------*/
/*
@gConnHandle:
@procid:
*/
gboolean 
qaDbusConnectionGetProcessId(DBusGConnection* gConnHandle, unsigned long* procid)
{
	DBusConnection* busconn = NULL;
	
	if(NULL==gConnHandle) {
		printf("ERROR: input gconnection handle is NULL.\n");
		procid = NULL;
		return FALSE;
	};

	if(NULL==procid) {
		printf("ERROR: input parameter uid is NULL.\n");
		return FALSE;
	};

	busconn = dbus_g_connection_get_connection (gConnHandle);
	if(NULL==busconn) {
		printf("FAIL: can't get dbus connection handle from dbus g connection handle.\n");
		procid = NULL;
		return FALSE;
	};

	if(TRUE==dbus_connection_get_unix_process_id (busconn, procid)) {
		return TRUE;
	}
	else {
		return FALSE;
	};
}

/*----------------------------qaDbusPeerServerNewProxy----------------------------*/
/*
@conn: 
@path:
@intf:
@connProxy:
*/
DBusGProxy*  
qaDbusPeerServerNewProxy(DBusGConnection* conn, const char* path, const char* intf)
{
	DBusGProxy* connProxy = NULL;
	
	if(NULL==conn) {
		printf("ERROR: input parameter conn is NULL.\n");
		return NULL;
	};

	if(NULL==path || NULL==intf) {
		printf("ERROR: input parameter path or interface is NULL.\n");
		return NULL;
	};
	
	connProxy = dbus_g_proxy_new_for_peer (conn, path, intf);
	if(NULL==connProxy) {
		printf("FAIL: call dbus_g_proxy_new_for_peer(%d, %s, %s) return invalid proxy point.\n", conn, path, intf);
		return NULL;
	};

	return connProxy;
}

/*----------------------------qaDbusProxySetDefaultTimeOut----------------------------*/
/*
@proxy:
@timeout: 
*/
void
qaDbusProxySetDefaultTimeOut(DBusGProxy* proxy, int timeout)
{
	if(NULL==proxy) {
		printf("ERROR: input parameter is NULL.\n");
		return;
	};
	
	return dbus_g_proxy_set_default_timeout	(proxy, timeout);
}

/*----------------------------qaDbusProxyGetBusName----------------------------*/
/*
@proxy: If the proxy is created by dbus_g_proxy_new_for_peer(), then, you should expect to get busName is NULL as return;
@busName: 
*/
void
qaDbusProxyGetBusName(DBusGProxy* proxy, char* busName)
{	
	if(NULL==proxy) {
		printf("ERROR: input parameter is NULL.\n");
		busName = NULL;
		return;
	};
	
	busName = dbus_g_proxy_get_bus_name (proxy);
	return;
}

/*----------------------------qaDbusProxyGetInterface----------------------------*/
/*
@proxy:
@Interface: 
*/
void
qaDbusProxyGetInterface(DBusGProxy* proxy, char* Interface)
{	
	if(NULL==proxy) {
		printf("ERROR: input parameter is NULL.\n");
		Interface = NULL;
		return;
	};
	
	Interface = dbus_g_proxy_get_interface (proxy);
	return;
}

/*----------------------------qaDbusProxyGetPath----------------------------*/
/*
@proxy:
@Path: 
*/
void
qaDbusProxyGetPath(DBusGProxy* proxy, char* Path)
{	
	if(NULL==proxy) {
		printf("ERROR: input parameter is NULL.\n");
		Path = NULL;
		return;
	};
	
	Path = dbus_g_proxy_get_path (proxy);
	return;
}

/*----------------------------qaThreadCreateTestThread----------------------------*/
/*
@ThreadFunc:
@ThreadData: 
@Joinable: 
*/
GThread* 
qaCreateTestThread(void* ThreadFunc, gpointer ThreadData, gboolean Joinable)
{
	GError* error = NULL;
	GThread* thrd = NULL;
	
	thrd = g_thread_create (ThreadFunc, ThreadData, Joinable, &error);
	if (NULL==thrd) {
		printf("WARN: create new thread failed\t error code is: <%s>.\n", error->message);
		g_error_free (error);
		return NULL;
	}
	else {
		printf("INFO: create new thread success.\n");
	};
	return thrd;
}

/*----------------------------qaGetRandomNumber----------------------------*/
/*
@Min:
@Max: 
*/
int 
qaGetRandomNumber(const int Min, const int Max)
{
	return Min + rand() / (RAND_MAX / (Max - Min + 1) + 1);
}

/*----------------------------qaGetSysClockMonoTime----------------------------*/
/*
@TimeNow: 
*/
void 
qaGetSysClockMonoTime(struct timespec TimeNow)
{
	clock_gettime (CLOCK_MONOTONIC, &TimeNow);
}

/*----------------------------qaGetSysTimesDiff----------------------------*/
/*
@Start: 
@End: 
*/
struct timespec 
qaGetSysTimesDiff(struct timespec Start, struct timespec End)
{
   struct timespec diff;
   if ((End.tv_nsec-Start.tv_nsec)<0) {
       diff.tv_sec = End.tv_sec-Start.tv_sec-1;
       diff.tv_nsec = 1000000000+End.tv_nsec-Start.tv_nsec;
   }
   else {
       diff.tv_sec = End.tv_sec-Start.tv_sec;
       diff.tv_nsec = End.tv_nsec-Start.tv_nsec;
   }
     return diff;
}

/*----------------------------qaCreateTimer----------------------------*/
GTimer* 
qaCreateTimer(void)
{
	GTimer* timer = NULL;
	
	timer = g_timer_new ();
	return NULL == timer ? NULL : timer;
}

/*----------------------------qaGetTimeDuration----------------------------*/
/*
@Timer:
@Seconds: 
*/
gdouble 
qaGetTimeDuration(GTimer* Timer, const char* FuncName)
{
	gdouble seconds;
	if (Timer) {
		seconds = g_timer_elapsed (Timer, NULL);
#if 0
		printf ("DURATION: Call Function: <%s>\tTime Cost: <%f>.\n", FuncName, seconds);
#endif
	};
	return seconds;
}

/*----------------------------qaDestroyTimer----------------------------*/
/*
@Timer: 
*/
void
qaDestroyTimer(GTimer* Timer)
{
	if (Timer) {
		return g_timer_destroy (Timer);
	};
}

/*---------------------customized data constructor----------------------------*/
GValue*
new_g_value(GType type)
{
  GValue* value = g_slice_new0 (GValue);
  g_value_init (value, type);
  return value;
}

void
free_g_value(GValue *value)
{
  g_value_unset (value);
  g_slice_free (GValue, value);
}

GType
dbus_ass_gtype(void)
{
  static GType t;
  static int initalized = 0;
  if (!initalized) {
    t = dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_STRING);
    initalized = 1;
  };
  return t;
}

GType 
dbus_asv_gtype(void)
{
  static GType t;
  static int initalized = 0;
  if (!initalized)
  {
    t = dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE);
    initalized = 1;
  }
  return t;
}

GHashTable*
dbus_ass_new(const char *first_key, ...)
{
  va_list args;
  gchar *key;
  gchar  *value , *temp_val;

  GHashTable *table = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)g_free);

  va_start(args, first_key);
  for (key=first_key; key!=NULL; key=va_arg(args, const char *)) {
    temp_val =  va_arg(args, const char *);
    value = g_strdup (temp_val);
    g_hash_table_insert (table, (char *)key, value);
  };
  va_end(args);

  return table;
}

GHashTable*
dbus_asv_new(const char *first_key, ...)
{
  va_list args;
  gchar *key;
  GType type;
  GValue *value;
  gchar *error = NULL;

  /* create a GHashTable */
  GHashTable *asv = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, (GDestroyNotify)free_g_value);

  va_start(args, first_key);
  for (key=first_key; key!=NULL; key=va_arg(args, const char*)) {
    type = va_arg(args, GType);

    value = new_g_value (type);
    G_VALUE_COLLECT(value, args, 0, &error);

    if (error!=NULL) {
      g_free (error);
      error = NULL;
      free_g_value (value);
      continue;
    };
    g_hash_table_insert (asv, (char*)key, value);
  };
  va_end(args);
  
  return asv;
}

void
dbus_asv_destroy(GHashTable* asv)
{
  g_hash_table_destroy(asv);
}

