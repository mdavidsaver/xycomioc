TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

LIBRARY_IOC = xy566

DBD += xy566Support.dbd

xy566_SRCS += drv566.c
xy566_SRCS += stc566.c
xy566_SRCS += seq566.c
xy566_SRCS += devbi566.c
xy566_SRCS += devmbbo566.c
xy566_SRCS += devai566.c
xy566_SRCS += devwf566.c
xy566_SRCS += iocsh566.c

xy566_LIBS += Com

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

