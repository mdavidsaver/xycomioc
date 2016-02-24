EPICS Support for XYCOM VME Cards
---------------------------------

Michael Davidsaver <mdavidsaver@bnl.gov>
NSLSII Project
Brookhaven National Lab.
Oct. 2009

Supported cards:

220 - Digital out

Fully supported

240 - Digitial in/out

bi Input and bo output
Direction selection

no mbbi/o
No support interrupt inputs or flag outputs

No reason they couldn't be added though.

566 - Analog Input

This is a complicated card.  Most uses will be fully
supported, but some advanced users may need to add
or change code related to the timing chip.
