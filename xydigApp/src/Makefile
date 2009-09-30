TOP=../..
include $(TOP)/configure/CONFIG

LIBRARY_IOC += xycomDigital
xycomDigital_LIBS += $(EPICS_BASE_IOC_LIBS)

DBD += xy210.dbd
xycomDigital_SRCS += drvXy210.c
xycomDigital_SRCS += devBiXVme210.c
xycomDigital_SRCS += devMbbiXVme210.c
xycomDigital_SRCS += devMbbiDirectXVme210.c

DBD += xy220.dbd
xycomDigital_SRCS += drvXy220.c
xycomDigital_SRCS += devBoXVme220.c
xycomDigital_SRCS += devMbboXVme220.c
xycomDigital_SRCS += devMbboDirectXVme220.c

DBD += xy240.dbd
xycomDigital_SRCS += drvXy240.c
xycomDigital_SRCS += devXy240.c

include $(TOP)/configure/RULES

