
dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

dbg566 = 2

xycom566setup(1, 0xf800, 0xa00000, 3, 0xf2, 1)

stc566simple(1, 2, 0x100)

# samples channges 0->3
# 1 sample each
# in order
seq566set(1, 0, 40, 1, 1)
seq566set(1, 1, 40, 2, 2)
seq566set(1, 2, 40, 3, 1)
seq566set(1, 3, 40, 4, 2)
seq566set(1, 4, 1, 5, 1)
seq566set(1, 5, 1, 6, 1)

iocInit()
