==== DW3000 Series UWB Driver + Wrapper ====

               STM32duino Port



Supports double-sided two-way ranging with STS-SDC, and PDoA.

NOTE: PDoA may require custom hardware. This library has not been tested on Qorvo development kits.



This driver currently only supports STM32F4 series microcontrollers.



Ported from DWT UWB Driver from DecaWave (now Qorvo).

 	Copyright 2016-2021 (c) DecaWave Ltd, Dublin, Ireland.

 	All rights reserved.





==================== !!! INSTALLATION INSTRUCTIONS !!! ====================

\*\*\* You will need the STM32 board package for Arduino.



1. Navigate to ...AppData\\Local\\Arduino15\\packages\\STMicroelectronics\\hardware\\stm32\\<latest version> and find the "boards.txt" file. Find where the "# NUCLEO\_F446RE board" section is and paste this below it:

# NUCLEO\_F446RE board (DW3 UWB Driver Support)
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3=Nucleo F446RE (DW3 UWB Support)
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.node=NODE\_F446RE
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.upload.maximum\_size=524288
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.upload.maximum\_data\_size=131072
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.mcu=cortex-m4
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.fpu=-mfpu=fpv4-sp-d16
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.float-abi=-mfloat-abi=hard
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.board=NUCLEO\_F446RE
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.series=STM32F4xx
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.product\_line=STM32F446xx
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.build.variant=STM32F4xx/F446RE\_DW3\_UWB
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.openocd.target=stm32f4x
Nucleo\_64.menu.pnum.NUCLEO\_F446RE\_DW3.debug.svd\_file={runtime.tools.STM32\_SVD.path}/svd/STM32F4xx/STM32F446.svd

2. Navigate to ...AppData\\Local\\Arduino15\\packages\\STMicroelectronics\\hardware\\stm32\\<latest version>\\variants\\STM32F4xx and paste the "F446RE\_DW3\_UWB" folder.
3. It is recommended to go to the Appdata\\Roaming folder and delete the contents in the "arduino-ide" (Note the lower case and hyphen!) folder to clear its cache.
4. When flashing, make sure to select "Nucleo F446RE (DW3 UWB Support)" in the Tools > Board part number menu.



===========================================================================



Version History:

0.1	(06/11/2025)	Internal Beta Release

1.0	(05/05/2026)	Initial Public Release



