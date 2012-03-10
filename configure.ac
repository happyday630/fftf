#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

# Require a minimum Autoconf version
AC_PREREQ([2.63])

# Define version for this component.
m4_define([MCA_TS_major_version], [0])
m4_define([MCA_TS_minor_version], [0])
m4_define([MCA_TS_micro_version], [1])
m4_define([MCA_TS_version],
		[MCA_TS_major_version.MCA_TS_minor_version.MCA_TS_micro_version])

# Mandatory Autoconf initialization - AC_INIT(PACKAGE, VERSION, BUG-REPORT-ADDRESS)
AC_INIT([MCA_TS], [MCA_TS_version], [hchen@opentv.com])
# Changing the Automake's Behavior
# foreign is Automake option to relax some GNU standard requirements.
AM_INIT_AUTOMAKE([color-tests] foreign)
LT_INIT
# Checks for programs
#AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)

# Checks for libraries
PKG_CHECK_MODULES(DBUS, dbus-1 >= 1.1, have_dbus=yes, have_dbus=no)
PKG_CHECK_MODULES(DBUS_GLIB, gobject-2.0 >= 2.26, have_glib=yes, have_glib=no)
PKG_CHECK_MODULES(MCA_TS, [dbus-1 dbus-glib-1 gobject-2.0 glib-2.0  gio-2.0])
# Checks for library functions

# Checks for header files
AC_HEADER_STDC
AC_CHECK_HEADERS([stdlib.h stdio.h string.h assert.h glib.h])

# Safety check for directory/file existence
AC_CONFIG_SRCDIR([test_main.c])

# Check for typedefs, structures, and compiler characteristics
AC_PROG_CC

# Output files
AC_CONFIG_FILES([
Makefile
example_component/Makefile
])
AC_OUTPUT

