SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\Program Files (x86)\STMicroelectronics\st_toolset\asm;C:\Python\Scripts\;C:\Python\;C:\Program Files\ARM\bin\win_32-pentium;C:\Program Files (x86)\Intel\Intel(R) Management Engine Components\iCLS\;C:\Program Files\Intel\Intel(R) Management Engine Components\iCLS\;C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\WINDOWS\System32\WindowsPowerShell\v1.0\;C:\Program Files (x86)\Intel\Intel(R) Management Engine Components\DAL;C:\Program Files\Intel\Intel(R) Management Engine Components\DAL;C:\Program Files (x86)\Intel\Intel(R) Management Engine Components\IPT;C:\Program Files\Intel\Intel(R) Management Engine Components\IPT;C:\WINDOWS\System32\OpenSSH\;C:\Program Files\MATLAB\R2018a\runtime\win64;C:\Program Files\MATLAB\R2018a\bin;C:\Program Files\TortoiseSVN\bin;C:\Program Files (x86)\Lua\5.1;C:\Program Files (x86)\Lua\5.1\clibs;C:\Python27;C:\strawberry\c\bin;C:\strawberry\perl\site\bin;C:\strawberry\perl\bin;C:\Program Files\ARM\RVD\Core\3.1\881\win_32-pentium\bin;C:\Program Files\ARM\Utilities\FLEXlm\10.8.5.0\1\win_32-pentium;C:\Program Files\ARM\Java\JRE\150\02\win_32-pentium\jre1.5.0_02\bin;C:\Program Files\ARM\RVCT\Programs\3.1\569\win_32-pentium;C:\Program Files\ARM\IDEs\Eclipse\Distribution\1.1\32\win_32-pentium\eclipse;C:\Program Files\ARM\RDI\Deprecated\1.3.1\1\windows;C:\ffmpeg\bin;C:\Program Files\Intel\WiFi\bin\;C:\Program Files\Common Files\Intel\WirelessCommon\;C:\Program Files (x86)\openocd\0.6.0\bin;C:\Program Files (x86)\CodeSourcery\Sourcery_CodeBench_Lite_for_ARM_EABI\bin;C:\Users\Chris.Lu\AppData\Local\Microsoft\WindowsApps;C:\Python;C:\Python\Scripts;C:\Python27;C:\Python27\Scripts;C:\Program Files (x86)\openocd\0.6.0\bin;
SET ARMCC5_ASMOPT=--diag_suppress=9931
SET ARMCC5_CCOPT=--diag_suppress=9931
SET ARMCC5_LINKOPT=--diag_suppress=9931
SET CPU_TYPE=STM32F103RC
SET CPU_VENDOR=STMicroelectronics
SET UV2_TARGET=T100-Bootloader
SET CPU_CLOCK=0x007A1200
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\main.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\fs_api.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\iap.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\delay.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\spi_flash_bsp.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\sys.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\uart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stmflash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\misc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_flash.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_gpio.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_it.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_rcc.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_spi.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_tim.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\system_stm32f10x.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_usart.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\stm32f10x_iwdg.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via "..\obj\core_cm3.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAsm" --Via "..\obj\startup_stm32f10x_hd._ia"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmLink" --Via "..\obj\bootloader.lnp"
"C:\Keil_v5\ARM\ARMCC\Bin\fromelf.exe" "..\obj\bootloader.axf" --i32combined --output "..\obj\bootloader.hex"
C:\Keil_v5\\ARM\ARMCC\bin\fromelf.exe --bin --output=bootloader.bin ..\obj\bootloader.axf
