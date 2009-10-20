TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

DB += xy566lnl.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

../O.Common/xy566lnl.db : ../xy566lnl.mac ../xy566base.db.part ../xy566gain.db.part \
 ../xy566wave.db.part ../xy566val.db.part

../O.Common/%.db: ../%.mac
	$(TOP)/dbpp.py -I.. -o $@ $<
