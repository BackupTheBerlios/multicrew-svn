all: local-install

cfg=$(OUTDIR)
fs=d:\fs9
remotefs=f:
gauge1=PMDG_737NG_Main
gauge2=PMDG_737NG_OHD_HYDRAULIC

gauge: gauge-stamp
gauge-stamp: multicrewgauge\$(cfg)\multicrewgauge.dll
	copy multicrewgauge\$(cfg)\multicrewgauge.dll $(fs)\multicrew
	echo > gauge-stamp
	
ui: ui-stamp
ui-stamp: multicrewui\$(cfg)\multicrewui.dll
	copy multicrewui\$(cfg)\multicrewui.dll $(fs)\modules
	echo > ui-stamp

core: core-stamp
core-stamp: multicrewcore\$(cfg)\multicrewcore.dll
	copy multicrewcore\$(cfg)\multicrewcore.dll $(fs)
	echo > core-stamp

remote-gauge: remote-gauge-stamp
remote-gauge-stamp: gauge
	copy multicrewgauge\$(cfg)\multicrewgauge.dll $(remotefs)
	echo > remote-gauge-stamp
	
remote-ui: remote-ui-stamp
remote-ui-stamp: multicrewui\$(cfg)\multicrewui.dll
	copy multicrewui\$(cfg)\multicrewui.dll $(remotefs)\modules
	echo > remote-ui-stamp

remote-core: remote-core-stamp
remote-core-stamp: multicrewcore\$(cfg)\multicrewcore.dll
	copy multicrewcore\$(cfg)\multicrewcore.dll $(remotefs)
	echo > remote-core-stamp

remote-install: remote-core remote-gauge remote-ui
local-install: core gauge ui core

install: local-install remote-install

clean:
	del ui-stamp core-stamp gauge-stamp remote-coreui-stamp remote-corecore-stamp remote-coregauge-stamp
