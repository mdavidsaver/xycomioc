TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

LIBRARY_IOC = xydig
DBD += xydig.dbd

xydig_DBD += base.dbd
xydig_DBD += dev240.dbd
xydig_DBD += dev220.dbd

xydig_SRCS += dev240.c
xydig_SRCS += dev220.c

# Finally link to the EPICS Base libraries
xydig_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

