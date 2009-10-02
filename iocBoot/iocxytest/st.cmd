
dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

xycom240setup(1,0xe000)

dbLoadRecords("db/240port.db","P=,C=1,N=0")

iocInit()
