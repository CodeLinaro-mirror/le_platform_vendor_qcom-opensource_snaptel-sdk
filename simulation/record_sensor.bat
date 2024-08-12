:: Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
:: SPDX-License-Identifier: BSD-3-Clause-Clear
:: Usage:
::   By default, this script captures sensor events for UNCALIBRATED_ACCELEROMETER AND
::   UNCALIBRATED_GYROSCOPE with both ROTATED and UNROTATED configurations enabled at
::   104hz sampling rate and 50 batchcount.
:: Note: Once captured, sort CSV by timestamp
@echo off
for /f "tokens=1-4 delims=/ " %%a in ('date /t') do set year=%%d

echo ##
echo ## Copyright (c) %year% Qualcomm Innovation Center, Inc. All rights reserved.
echo ## SPDX-License-Identifier: BSD-3-Clause-Clear
echo ##
adb shell "sensor_test_app -r 2>&1 >/dev/null | grep '^###' | sed 's/\#\#\#//g' "
pause
