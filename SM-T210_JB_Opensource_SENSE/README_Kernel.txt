HOW TO BUILD KERNEL 3.4.5 FOR SM-T210

1. How to Build
	(1) get Toolchain
	Visit http://www.codesourcery.com/, download and install Sourcery G++ Lite 2009q3-68 toolchain for ARM EABI.
	(2) Extract kernel source and move into the top directory.
	$ cd kernel
	$ make all

2. Output files
	- Kernel : kernel/common/arch/arm/boot/zImage
	
3. How to make .tar binary for downloading into target.
	- change current directory to kernel/common/arch/arm/boot
	- type following command
	$ tar cvf SM-T210_Kernel_JellyBean.tar zImage
