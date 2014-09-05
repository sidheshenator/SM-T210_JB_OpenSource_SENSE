How to build Mobule for Platform
- It is only for modules are needed to using Android build system.
- Please check its own install information under its folder for other module.

[Step to build]
1. Get android open source.
    : version info - Android 4.1.2
    ( Download site : http://source.android.com )

2. Copy module that you want to build - to original android open source
   If same module exist in android open source, you should replace it. (no overwrite)
   
  # It is possible to build all modules at once.
  
3. In case of 'external\bluetooth',
   you should add following text in 'build\target\board\generic\BoardConfig.mk'
BOARD_HAVE_BLUETOOTH := true

4. In case of 'vendor\marvell\external\alsa-lib',
   you should add following text in 'build\target\board\generic\BoardConfig.mk'
BOARD_USES_ALSA_AUDIO := true

5. You should add module name to 'PRODUCT_PACKAGES' in 'build\target\product\core.mk' as following case.
	case 1) e2fsprog : should add 'e2fsck' to PRODUCT_PACKAGES
	case 2) libexifa : should add 'libexifa' to PRODUCT_PACKAGES
	case 3) libjpega : should add 'libjpega' to PRODUCT_PACKAGES
	case 4) KeyUtils : should add 'libkeyutils' to PRODUCT_PACKAGES
	case 5) bluetooth : should add 'audio.a2dp.default \ avinfo \ hcitool \ l2ping \ rfcomm' to PRODUCT_PACKAGES
	case 6) alsa-lib : should add 'libasound' to PRODUCT_PACKAGES
    case 7) mrvl_dut : should add 'mrvl_dut' to PRODUCT_PACKAGES
    case 8) i2c-tools : should add 'i2cdetect \ i2cdump \ i2cget \ i2cset' to PRODUCT_PACKAGES
    case 9) sysfsutils : should add 'libsysfs' to PRODUCT_PACKAGES
    case 10) cpufrequtils : should add 'libcpufreq' to PRODUCT_PACKAGES
    case 11) wireless_tools : should add 'iwconfig \ iwevent \ iwgetid \ iwlist \ iwpriv \ iwspy' to PRODUCT_PACKAGES
    case 12) android-vnc-server : should add 'androidvncserver' to PRODUCT_PACKAGES
    case 13) PowerDaemon : should add 'powerdaemon' to PRODUCT_PACKAGES
                  
ex.) [build\target\product\core.mk] - add all module name for case 1 ~ 14 at once
	PRODUCT_PACKAGES += \
		e2fsck \
		libexifa \
		libjpega \
		libkeyutils \
		audio.a2dp.default \ avinfo \ hcitool \ l2ping \ rfcomm \
		libasound \
		mrvl_dut \
		i2cdetect \ i2cdump \ i2cget \ i2cset \
		libsysfs \
		libcpufreq \
		iwconfig \ iwevent \ iwgetid \ iwlist \ iwpriv \ iwspy \
		androidvncserver \
		powerdaemon

6. excute build command
   ./build.sh user
