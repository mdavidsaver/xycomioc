TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC   = xytest

# xytest.dbd will be created and installed
DBD += xytest.dbd

# xytest.dbd will be made up from these files:
xytest_DBD += base.dbd
xytest_DBD += xy566Support.dbd
xytest_DBD += xydig.dbd

xytest_LIBS += xy566 xydig

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

