TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

#=============================
# Build the IOC application

PROD_IOC = xy566
# xy566.dbd will be created and installed
DBD += xy566.dbd

# xy566.dbd will be made up from these files:
xy566_DBD += base.dbd

# Include dbd files from all support applications:
#xy566_DBD += xxx.dbd

# Add all the support libraries needed by this IOC
#xy566_LIBS += xxx

# xy566_registerRecordDeviceDriver.cpp derives from xy566.dbd
xy566_SRCS += xy566_registerRecordDeviceDriver.cpp

# Build the main IOC entry point on workstation OSs.
xy566_SRCS_DEFAULT += xy566Main.cpp
xy566_SRCS_vxWorks += -nil-

# Add support from base/src/vxWorks if needed
#xy566_OBJS_vxWorks += $(EPICS_BASE_BIN)/vxComLibrary

# Finally link to the EPICS Base libraries
xy566_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

