
dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

xy220setup(1,0x4000)

xy240setup(1,0x6000)

dbLoadTemplate("db/xy220.substitutions")
dbLoadTemplate("db/xy240.substitutions")

iocInit()
