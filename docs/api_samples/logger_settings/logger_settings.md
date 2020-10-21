# Configuring Logs from the SDK

Please follow below steps to configure Logger settings.

Telematics SDK provides a configurable logger module that can be used to log messages from Telematics SDK library and applications at desired threshold levels into device console, diag and optionally into a log file.

By default, *tel.conf* will be placed under /etc location

The configuration file called "appName.conf" or "tel.conf" is used to configure logger settings such as logging threshold, enable/disable file logging and to change the log file name. These file have to be updated to override default behavior. These configuration file should be copied either in /etc or the folder where the application is running.

To modify tel.conf file under /etc, you need to mount partition on MDM A7 processor

   ~~~~~~{.sh}
   adb shell mount -o rw,remount /
   ~~~~~~

**NOTE:** The file path where the log file will be written to, need to be in a writable partition, accessible to the application that is running.

In the case of MDMs A7 processor the /data partition is writable.

Here is how the platform searches for the configuration file. If configuration file is found use the same to configure logger settings else keep continue to search in below order.
-  Search for appName.conf in /etc folder. (i.e. telsdk_console_app.conf)
-  Search for appName.conf in the folder that contains the application.
-  Search for tel.conf in etc folder.
-  Search for tel.conf in the folder that contains the application.

This allows flexibility for app's to either share the same log file or keep each apps log file separate.

### 1. Console and file level logging

CONSOLE_LOG_LEVEL, FILE_LOG_LEVEL specifies the threshold for console log messages. Possible LOG_LEVEL values are NONE, PERF, ERROR, WARNING, INFO, DEBUG

   ~~~~~~{.sh}
   # NONE - No logging.
   # PERF - Prints messages with nanoseconds precision timestamp.
   # ERROR - Very minimal logging.Prints perf and error messages only.
   # WARNING - Prints perf, error and warning messages.
   # INFO - Prints perf, errors, warning and information messages.
   # DEBUG - Full logging including debug messages.It is intended for debugging purposes only.

   CONSOLE_LOG_LEVEL=INFO
   FILE_LOG_LEVEL=DEBUG
   DIAG_LOG_LEVEL=DEBUG
   ~~~~~~
**NOTE:** For an applicaiton to be able to log to the tel.log file, it should have "system" linux group permissions.

### 2. Diag level logging

DIAG_LOG_LEVEL specifies the threshold for logs messages displayed in QXDM. Possible LOG_LEVEL values are NONE, PERF, ERROR, WARNING, INFO, DEBUG.
The mapping of SDK log levels to QXDM log levels in shown below:
   ~~~~~~{.sh}
   # SDK Log Levels --> QXDM LOG Levels
   # PERF --> FATAL (MSG_LEGACY_FATAL)
   # ERROR --> ERROR (MSG_LEGACY_ERROR)
   # WARNING --> HIGH (MSG_LEGACY_HIGH)
   # INFO --> MED (MSG_LEGACY_MED)
   # DEBUG --> LOW (MSG_LEGACY_LOW)
   ~~~~~~
**NOTE:** For an applicaiton to be able to log to the Diag, it should have "diag" linux group permissions.

### 3. Log filtering

TELUX_LOG_COMPONENT_FILTER allows one or more whitelist which SDK technology domain should be logged
   ~~~~~~{.sh}
   # 0 - All logs are printed.
   # 1 - Audio logs are printed.
   # 2 - CV2X logs are printed.
   # 3 - Data logs are printed.
   # 4 - Location logs are printed.
   # 5 - Power logs are printed.
   # 6 - Telephony logs are printed.
   # 7 - Thermal logs are printed.

   # For logging all component
   # use TELUX_LOG_COMPONENT_FILTER= 0

   # For logging more than one component like cv2x and audio (comma separated)
   # use TELUX_LOG_COMPONENT_FILTER= 2,1
   ~~~~~~
### 4. Set Max file size

MAX_LOG_FILE_SIZE specifies the maximum allowed size(in bytes) of the log file
When the log file reaches its maximum size, it is saved as tel.log.backup.
-  If the log file again reaches the max, it will be saved again overwriting the previous tel.log.backup file.
-  So at a given time only one tel.log and tel.backup will exist in system.
-  Default MAX_LOG_FILE_SIZE is 5 Mega Bytes.

   ~~~~~~{.sh}
   MAX_LOG_FILE_SIZE=5242880
   ~~~~~~

### 5. Prefix date and time for the log message

Used to prefix date and time on every log Message

   ~~~~~~{.sh}
   # FALSE - logs with filename and line number, this is default option
   # TRUE - logs with date, time, filename and line number

   LOG_PREFIX_DATE_TIME=TRUE
   ~~~~~~

### 6. Set log file path

Specifies the path of the log file. In an external application processor, the path needs to be in a writable partition. If this default path does not exist in the system or it is not writable, this path needs to be updated accordingly.
   ~~~~~~{.sh}
   LOG_FILE_PATH=/data/vendor/telsdk
   ~~~~~~

### 7. Set log file name

Specifies the name of the log file to be used

   ~~~~~~{.cpp}
   LOG_FILE_NAME=tel.log
   ~~~~~~
