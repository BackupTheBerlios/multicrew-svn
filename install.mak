all: install

cfg=$(OUTDIR)
fs=d:\fs9
remotefs=m:
gauge1=PMDG_737NG_Main
gauge2=PMDG_737NG_OHD_HYDRAULIC

prepare-gauge: prepare-gauge-stamp
prepare-gauge-stamp: prepare gauge TCAS2v7.dll
	cd $(fs)
	multicrewpanelprepare Aircraft\PMDG737-700\panel\Panel.CFG
#petite -r* multicrewPMDG_737NG_Main.dll
#del multicrewPMDG_737NG_Main.dll.bak
	cd $(MAKEDIR)
	echo > prepare-gauge-stamp

gauge: gauge-stamp
gauge-stamp: multicrewgauge\$(cfg)\multicrewgauge.dll
	copy multicrewgauge\$(cfg)\multicrewgauge.dll $(fs)\multicrew
	echo > gauge-stampy
	
ui: ui-stamp
ui-stamp: multicrewui\$(cfg)\multicrewui.dll
	copy multicrewui\$(cfg)\multicrewui.dll $(fs)\modules
	echo > ui-stamp

core: core-stamp
core-stamp: multicrewcore\$(cfg)\multicrewcore.dll
	copy multicrewcore\$(cfg)\multicrewcore.dll $(fs)
	echo > core-stamp

prepare: prepare-stamp
prepare-stamp: multicrewprepare\$(cfg)\multicrewprepare.exe multicrewprepare\$(cfg)\multicrewpanelprepare.exe
	copy multicrewprepare\$(cfg)\multicrewprepare.exe $(fs)
	copy multicrewprepare\$(cfg)\multicrewpanelprepare.exe $(fs)
	echo > prepare-stamp

remote-gauge: remote-gauge-stamp
remote-gauge-stamp: prepare-gauge
	copy $(fs)\gauges\multicrew*.gau $(remotefs)\gauges
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
local-install: core gauge ui core prepare prepare-gauge

install: local-install remote-install

clean:
	del ui-stamp core-stamp gauge-stamp remote-coreui-stamp remote-corecore-stamp remote-coregauge-stamp
