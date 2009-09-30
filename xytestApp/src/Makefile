TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC_vxWorks = xytest
PROD_IOC_RTEMS   = xytest

# xytest.dbd will be created and installed
DBD += xytest.dbd

# xytest.dbd will be made up from these files:
xytest_DBD += base.dbd
xytest_DBD += xy210.dbd
xytest_DBD += xy220.dbd
xytest_DBD += xy240.dbd
xytest_DBD += xy566.dbd

xytest_LIBS += xycomAnalog xycomDigital

# xytest_registerRecordDeviceDriver.cpp derives from xytest.dbd
xytest_SRCS += xytest_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
xytest_SRCS_DEFAULT += xytestMain.cpp
xytest_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#xytest_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
xytest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

