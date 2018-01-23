@echo off
set PATH=%PATH%;c:\__DANE__\tools\CodeSourcery\Sourcery_CodeBench_Lite_for_ARM_EABI\bin\
set RM=cs-rm -f
cs-make clean
cs-make -j10 VT100=y