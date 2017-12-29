# Configuring Logs from the SDK

*Quick steps:* Please follow below steps to configure Logger settings.

Telematics SDK provides a configurable logger module that can be used to log messages from Telematics SDK library at desired threshold levels into device console and optionally into a log file.

A configuration file called "tel.conf" is provided to configure logger settings such as logging threshold, enable/ disable file logging and to change the log file name.

File "tel.conf" is located in project workspace in "<BASE_DIR>/config" and in "/data" folder on the target device.

In order to change the default behavior of the Telematics SDK library, the settings in "tel.conf" file need to be updated as required and the updated file should be copied into the same folder where the application using Telematics SDK library is located.

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
-  When max size is reached, logger backs up the log file once, for example: tapi.log will be renamed to tapi.log.backup and a new log file will be created.
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

### 5. set log file name

Specifies the name of the log file to be used

   ~~~~~~{.cpp}
   LOG_FILE_NAME=tel.log
   ~~~~~~
