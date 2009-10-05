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

# xydig_registerRecordDeviceDriver.cpp derives from xydig.dbd
xydig_SRCS += xydig_registerRecordDeviceDriver.cpp
xydig_SRCS += dev240.c
xydig_SRCS += dev220.c

# Build the main IOC entry point on workstation OSs.
xydig_SRCS_DEFAULT += xydigMain.cpp
xydig_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#xydig_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
xydig_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

