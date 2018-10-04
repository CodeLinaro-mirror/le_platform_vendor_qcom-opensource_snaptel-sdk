# Configuring Logs from the SDK

Please follow below steps to configure Logger settings.

Telematics SDK provides a configurable logger module that can be used to log messages from Telematics SDK library at desired threshold levels into device console and optionally into a log file.

By default, both console logging and file logging are set to "NONE" log level, *tel.conf* will be placed under /etc location

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

CONSOLE_LOG_LEVEL, FILE_LOG_LEVEL specifies the threshold for console log messages Possible LOG_LEVEL values are NONE, ERROR, WARNING, INFO, DEBUG

   ~~~~~~{.sh}
   # NONE - No logging.
   # ERROR - Very minimal logging.Prints error messages only.
   # WARNING - Prints both error and warning messages.
   # INFO - Prints errors, warning and information messages.
   # DEBUG - Full logging including debug messages.It is intended for debugging purposes only.

   CONSOLE_LOG_LEVEL=INFO
   FILE_LOG_LEVEL=DEBUG
   ~~~~~~

### 2. Set Max file size

MAX_LOG_FILE_SIZE specifies the maximum allowed size(in bytes) of the log file
-  When max size is reached, logger backs up the log file once, for example: tel.log will be renamed to tel.log.backup and a new log file will be created.
-  Default MAX_LOG_FILE_SIZE is 5 Mega Bytes

   ~~~~~~{.sh}
   MAX_LOG_FILE_SIZE=5242880
   ~~~~~~

### 3. Prefix date and time for the log message

Used to prefix date and time on every log Message

   ~~~~~~{.sh}
   # FALSE - logs with filename and line number, this is default option
   # TRUE - logs with date, time, filename and line number

   LOG_PREFIX_DATE_TIME=TRUE
   ~~~~~~

### 4. Set log file path

Specifies the path of the log file

   ~~~~~~{.sh}
   LOG_FILE_PATH=/data
   ~~~~~~

### 5. Set log file name

Specifies the name of the log file to be used

   ~~~~~~{.cpp}
   LOG_FILE_NAME=tel.log
   ~~~~~~
