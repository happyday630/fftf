/*test_common.h*/
#ifndef __TEST_COMMON_H__
#define __TEST_COMMON_H__
#include <dbus/dbus-glib.h>
#include <stdlib.h>
#include <glib.h>

typedef GHashTable dbus_asv;

typedef struct 
{	
	DBusGProxy* pProxy;
	GHashTable* data;
	char* UUID;
}SendMsgHelperData;

/*---------------------test common library---------------------*/
DBusGConnection* qaDbusPeerServerNewConnect(const char* serverAddr);
gboolean qaIsDbusConnectionConnected(DBusGConnection* gconnHandle);
gboolean qaIsDbusConnectionAnonymous(DBusGConnection* gConnHandle);
gboolean qaDbusConnectionGetUnixUser(DBusGConnection* gConnHandle, unsigned long* uid);
long qaDbusConnectionGetMaxMsgSize(DBusGConnection* gConnHandle);
gboolean qaDbusConnectionGetProcessId(DBusGConnection* gConnHandle, unsigned long* procid);
DBusGProxy* qaDbusPeerServerNewProxy(DBusGConnection* conn, const char* path, const char* intf);
void qaDbusProxySetDefaultTimeOut(DBusGProxy* proxy, int timeout);
void qaDbusProxyGetBusName(DBusGProxy* proxy, char* busName);
void qaDbusProxyGetInterface(DBusGProxy* proxy, char* Interface);
void qaDbusProxyGetPath(DBusGProxy* proxy, char* Path);
GThread* qaCreateTestThread(void* ThreadFunc, gpointer ThreadData, gboolean Joinable);
int qaGetRandomNumber(const int Min, const int Max);
void qaGetSysClockMonoTime(struct timespec TimeNow);
struct timespec qaGetSysTimesDiff(struct timespec Start, struct timespec End);
GTimer* qaCreateTimer(void);
gdouble qaGetTimeDuration(GTimer* Timer, const char* FuncName);
void qaDestroyTimer(GTimer* Timer);
/*---------------------customerized data constructor----------------------*/
GValue* new_g_value(GType type);
void free_g_value(GValue *value);
GType dbus_ass_gtype(void);
GType dbus_asv_gtype(void);
GHashTable* dbus_ass_new(const char *first_key, ...);
GHashTable* dbus_asv_new(const char *first_key, ...);
void dbus_asv_destroy(const dbus_asv *asv);
#endif
