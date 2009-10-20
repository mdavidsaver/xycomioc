
cd "/xycomioc/iocBoot/ioc566test"

< cdCommands

cd topbin

ld < xytest.munch

cd top

dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

dbg566 = 2

############## 240 DIO card ##################

xycom240setup(3,0xe000)

############## 220 DO card ##################

xycom220setup(4,0x3000)

############### ADC card 1 ####################

xycom566setup(1, 0xf800, 0xa00000, 3, 0xf2, 1)

#stc566simple(1, 2, 0x100)
stc566seqmulti(1, 2, 0x100, 0x4800)

# samples channels 0->3
# 40 sample each
# in order w/ breaks between 0/1, 1/2, and 2/3
#
# sample channels 4,5
# 1 sample each
seq566set(1, 0, 40, 1, 1, 40)
seq566set(1, 1, 40, 2, 1, 40)
seq566set(1, 2, 40, 3, 1, 40)
seq566set(1, 3, 40, 4, 1)
seq566set(1, 4, 1, 5, 1)
seq566set(1, 5, 1, 6, 1)

############### ADC card 2 ####################

xycom566setup(2, 0xf400, 0xb00000, 3, 0xe0, 1)

#stc566simple(2, 2, 0x100)
stc566seqmulti(2, 2, 0x100, 0x4800)

# samples channels 0->3
# 40 sample each
# in order w/ breaks between 0/1, 1/2, and 2/3
#
# sample channels 4,5
# 1 sample each
seq566set(2, 0, 40, 1, 1, 40)
seq566set(2, 1, 40, 2, 2, 40)
seq566set(2, 2, 40, 3, 3, 40)
seq566set(2, 3, 40, 4, 4)
seq566set(2, 4, 1, 5, 1)
seq566set(2, 5, 1, 6, 1)

xycom566finish()

################# Records ###################

dbLoadRecords("db/xy566lnl.db","P=spec:adc1:,C=1")
dbLoadRecords("db/xy566lnl.db","P=spec:adc2:,C=2")

dbLoadRecords("db/xy240.db","P=spec:dio1:,C=3")

dbLoadRecords("db/xy220.db","P=spec:do1:,C=4")

iocInit()

