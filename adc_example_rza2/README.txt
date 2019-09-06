This is an example of reading an ADC from application space by directly
accessing the ADC registers without a kernel driver.

Since there is no POT on the RZ/A2M EVB, this uses buttons (switches) SW4 and SW55
which are connected to an ADC input through resistors ladder.


-- Pin setup --
Note that you must first apply the patch to the kernel that will configure the pin as ADC.
$ cd rza_linux-4.19_bsp/output/linux-4.19
$ patch -p1 -i ../../adc_example_rza2/0001-board-rza2mvb-pin-setup-for-ADC.patch
$ cd ../../
$ ./build.sh kernel xipImage
$ ./build.sh jlink xipImage 0x20200000


Or, you can 'cheat' and just add the ADC pin setup to an existing Device Tree node.
  FILE: rza_linux-4.19_bsp/output/linux-4.19/arch/arm/boot/dts/r7s9210-rza2mevb.dts
For example, we know that the SCIF4 serial console will get used, so we can
just add the ADC pin.

	/* Serial Console */
	scif4_pins: serial4 {
		pinmux = <RZA2_PINMUX(PORT9, 0, 4)>,	/* TxD4 */
+			 <RZA2_PINMUX(PORT5, 6, 1)>,	/* AN6 */
			 <RZA2_PINMUX(PORT9, 1, 4)>;	/* RxD4 */
	};

Then we just need to recompile the device tree and reporgram it.
$ ./build.sh kernel dtbs
$ ./build.sh jlink dtb

