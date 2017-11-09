# Configure Logger

*Quick steps:* Please follow below steps to configure Logger settings.

Telematics SDK provides a configurable logger module that can be used to log messages from Telematics SDK library at desired threshold levels into device console and optionally into a log file.

A configuration file called "tel.conf" is provided to configure logger settings such as logging threshold, enable/ disable file logging and to change the log file name.

File "tel.conf" is located in project workspace in "<BASE_DIR>/config" and in "/data" folder on the target device.

By default, both console and file logging are enabled, the log level is set to "INFO" level.

In order to change the default behavior of the Telematics SDK library, the settings in "tel.conf" file need to be updated as required and the updated file should be copied into the same folder where the application using Telematics SDK library is located.

### 1. LOGGING_TO_FILE_ENABLED specifies if logging to a file is required or not ###
Possible values are TRUE or FALSE
~~~~~~{.cpp}
LOGGING_TO_FILE_ENABLED=TRUE
~~~~~~

### 2. LOG_LEVEL specifies the threshold for log messages ###
Possible LOG_LEVEL values are NONE, ERROR, WARNING, INFO, DEBUG
   
   * NONE - No logging.
~~~~~~{.cpp}
LOG_LEVEL=NONE
~~~~~~
   * ERROR - Very minimal logging. Prints error messages only.
~~~~~~{.cpp}
LOG_LEVEL=ERROR
~~~~~~
   * WARNING - Prints both error and warning messages.
~~~~~~{.cpp}
LOG_LEVEL=WARNING
~~~~~~
   * INFO - Prints errors, warning and information messages. This is the recommended setting for release builds.
~~~~~~{.cpp}
 LOG_LEVEL=INFO
~~~~~~
   * DEBUG - Full logging including debug messages. It is intended for debugging purposes only.
~~~~~~{.cpp}
LOG_LEVEL=DEBUG
~~~~~~

### 3. LOG_FILE_PATH specifies the path of the log file ###
~~~~~~{.cpp}
LOG_FILE_PATH=/data
~~~~~~

### 4. LOG_FILE_NAME specifies the name of the log file to be used ###
~~~~~~{.cpp}
LOG_FILE_NAME=tel.log
~~~~~~
