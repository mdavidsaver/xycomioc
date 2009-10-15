## Example vxWorks startup file

## The following is needed if your board support package doesn't at boot time
## automatically cd to the directory containing its startup script
cd "/xycomioc/iocBoot/iocxytest"

< cdCommands
#< ../nfsCommands

cd topbin

## You may have to change baseTest to something else
## everywhere it appears in this file
ld < xytest.munch

## Register all support components
cd top
dbLoadDatabase("dbd/xytest.dbd")
xytest_registerRecordDeviceDriver(pdbbase)

xycom240setup(1,0xe000)

dbLoadRecords("db/240port.db","P=,C=1,N=0")

iocInit()
