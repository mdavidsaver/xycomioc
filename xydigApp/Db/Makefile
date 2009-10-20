TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install) into <top>/db
# databases, templates, substitutions like this
DB += xy240.db
DB += xy220.db

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_template = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

../O.Common/xy240.db : ../xy240.mac ../240port.db.part ../xybo.db.part ../xybi.db.part

../O.Common/xy220.db : ../xy220.mac ../xybo.db.part

../O.Common/%.db: ../%.mac
	$(TOP)/dbpp.py -I.. -o $@ $<
