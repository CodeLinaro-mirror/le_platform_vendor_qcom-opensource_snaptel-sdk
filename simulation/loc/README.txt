Simulation Folder contains the Simulation of Telematics SDK APIs. Currently Location APIs are simulated. The Simulation supports multiple LocationManager Objects and multiple listeners. Only one object of LocationConfigurator and DgnssManager are created and provided in the getInstance. Though multiple objects and listeners for location manager are supported, the code has been tested only with location_test_app which uses single object.

Location Simulation uses a configuration file called StubConfig. This can be located at current working directory or /etc/ or /usr/local/bin/ as per order of search for the file.

Simulation of Reports is mainly of two types and can be configured using StubConfig:

1. REPORT_SIMULATION_TYPE = 0 (Default) : In this case Canned values for various reports are used. The values do not change with time. Duration parameter requested in the location_test_app while Starting reports do have change the values.

2. REPORT_SIMULATION_TYPE = 1 : This is file based. A log File PRE-RECORDED_LOCATION_DATA.csv is used for simulation. This has been captured with a Kinematics Server and provided. Data in each row is spaced 100 msecs apart. A duration of 1000 msecs, gives out data in the file 10 rows apart. The reports has changing data based on the file. Data not in the file is shown with default values: 0,NAN(Not-A-Number),etc. If file is not available, all values are shown with default.


Location Configurator is also simulated with the following notes.
a. All simulation is local, in that if multiple instances of location_test_app are running, a configuration done on one process does not get reflected in others but only locally. For ex: value configured through Configure minSV elevation can be obtained in the Request minsv elevation in the same program, but not on other instances of program running.
b. Parameters are consumed, but does not affect any reports.

