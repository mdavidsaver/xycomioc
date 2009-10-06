TOP=../..
include $(TOP)/configure/CONFIG

DBD = xy566.dbd

LIBRARY_IOC += xycomAnalog
xycomAnalog_LIBS += $(EPICS_BASE_IOC_LIBS)
xycomAnalog_SRCS += devAiXy566Di.c
xycomAnalog_SRCS += devAiXy566DiL.c
xycomAnalog_SRCS += devAiXy566Se.c
xycomAnalog_SRCS += devWfXy566Sc.c
xycomAnalog_SRCS += drvXy566.c

include $(TOP)/configure/RULES

