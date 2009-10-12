
dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

dbg566 = 2

xycom566setup(1, 0xf800, 0xa00000, 3, 0xf2, 1)
#xycom566setup(2, 0xf400, 0xb00000, 3, 0xe0, 1)

stc566simple(1, 2, 0x100)

# samples channges 0->3
# 1 sample each
# in order
seq566set(1, 0, 40, 1, 1)
#seq566set(1, 1, 40, 2, 2)
#seq566set(1, 2, 40, 3, 3)
#seq566set(1, 3, 40, 4, 4)
#seq566set(1, 4, 1, 5, 1)
#seq566set(1, 5, 1, 6, 1)

xycom566finish()

dbLoadRecords("db/xy566base.db","P=,C=1")

dbLoadRecords("db/xy566gain.db","P=,CH=ch1:,C=1,N=0")
dbLoadRecords("db/xy566val.db","P=,CH=ch1,C=1,N=0")
dbLoadRecords("db/xy566wave.db","P=,CH=ch1:wave,C=1,N=0,L=40")

#dbLoadRecords("db/xy566gain.db","P=,CH=ch2:,C=1,N=1")
#dbLoadRecords("db/xy566val.db","P=,CH=ch2,C=1,N=1")
#dbLoadRecords("db/xy566wave.db","P=,CH=ch2:wave,C=1,N=1,L=40")

#dbLoadRecords("db/xy566gain.db","P=,CH=ch3:,C=2,N=0")
#dbLoadRecords("db/xy566val.db","P=,CH=ch3,C=1,N=2")
#dbLoadRecords("db/xy566wave.db","P=,CH=ch3:wave,C=1,N=2,L=40")

#dbLoadRecords("db/xy566gain.db","P=,CH=ch4:,C=1,N=3")
#dbLoadRecords("db/xy566val.db","P=,CH=ch4,C=1,N=3")
#dbLoadRecords("db/xy566wave.db","P=,CH=ch4:wave,C=1,N=3,L=40")

iocInit()
