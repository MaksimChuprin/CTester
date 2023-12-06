@if "%1" equ "" goto error1

@cd Release

@if not exist "CAP_TESTER_STM32L152RCT6.hex" goto error2

@set fullfilename=CT_rev_%1.hex
@copy "CAP_TESTER_STM32L152RCT6.hex" "%fullfilename%"
@exit 0

:error1
@echo Not enought input parameters
@exit 1

:error2
@echo Not exist CAP_TESTER_STM32L152RCT6.hex
@exit 2
