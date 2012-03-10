AM_CPPFLAGS = $(MCA_TS_CFLAGS)

AM_LDADD = $(MCA_TS_LIBS)

AM_LDFLAGS = $(MCA_TS_LIBS)

SUBDIRS = example_component tss userauth resman MPM_SFS .

bin_PROGRAMS =	MCA_TS 

MCA_TS_SOURCES = 	frame.c	\
					frame.h	\
					utl.c		\
					test_common.c \
					test_common.h \
					test_main.c

MCA_TS_LDADD=$(AM_LDADD)				
MCA_TS_LDADD+=example_component/libexample_component.la

CLEANFILES = $(BUILT_SOURCES)

