all: install

cfg=$(OUTDIR)
fs=c:\fs\fs9
remotefs=m:
gauge=PMDG_737NG_Main


TCAS2v7.dll:
	copy $(fs)\TCAS2v7.dll .

prepare-gauge: prepare-gauge-stamp
prepare-gauge-stamp: multicrewgauge\$(cfg)\multicrewgauge.dll TCAS2v7.dll
	multicrewprepare\$(cfg)\multicrewprepare.exe $(fs)\gauges\$(gauge).gau multicrewgauge\$(cfg)\multicrewgauge.dll multicrew$(gauge).dll
#petite -r* multicrewPMDG_737NG_Main.dll
#del multicrewPMDG_737NG_Main.dll.bak
	echo > prepare-gauge-stamp

gauge: gauge-stamp
gauge-stamp: prepare-gauge
	copy multicrew$(gauge).dll $(fs)\gauges\multicrewh$(gauge).gau
	copy multicrew$(gauge).dll $(fs)\gauges\multicrewc$(gauge).gau
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
remote-gauge-stamp: prepare-gauge
	copy multicrew$(gauge).dll $(remotefs)\gauges\multicrewh$(gauge).gau
	copy multicrew$(gauge).dll $(remotefs)\gauges\multicrewc$(gauge).gau
	echo > remote-gauge-stamp
	
remote-ui: remote-ui-stamp
remote-ui-stamp: multicrewui\$(cfg)\multicrewui.dll
	copy multicrewui\$(cfg)\multicrewui.dll $(fs)\modules
	copy multicrewui\$(cfg)\multicrewui.dll $(remotefs)\modules
	echo > remote-ui-stamp

remote-core: remote-core-stamp
remote-core-stamp: multicrewcore\$(cfg)\multicrewcore.dll
	copy multicrewcore\$(cfg)\multicrewcore.dll $(fs)
	copy multicrewcore\$(cfg)\multicrewcore.dll $(remotefs)
	echo > remote-core-stamp

remote-install: remote-core remote-gauge remote-ui
local-install: core gauge ui core

install: local-install 
#remote-install

clean:
	del ui-stamp core-stamp gauge-stamp remote-coreui-stamp remote-corecore-stamp remote-coregauge-stamp
