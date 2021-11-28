Using Location Configurator APIs {#location_services_configurator}
==================================================================

# Using Location Configurator APIs

Please follow below steps to use Configurator APIs

### 1. Implement a command response function ###

   ~~~~~~{.cpp}
    void CmdResponse(ErrorCode error) {
        if (error == ErrorCode::SUCCESS) {
            std::cout << " Command executed successfully" << std::endl;
        }
        else {
            std::cout << " Command failed\n errorCode: " << static_cast<int>(error) << std::endl;
        }
    }
   ~~~~~~

### 2. Get the LocationFactory instance ###

   ~~~~~~{.cpp}
    auto &locationFactory = LocationFactory::getInstance();
   ~~~~~~

### 3. Get LocationConfigurator instance ###

   ~~~~~~{.cpp}
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto locConfigurator_ = locationFactory.getLocationConfigurator([&](ServiceStatus status) {
        prom.set_value(status);
    });
   ~~~~~~

### 4. Wait for the Location Config. initialization ###

   ~~~~~~{.cpp}
   ServiceStatus managerStatus = locConfigurator_->getServiceStatus();
    if (managerStatus != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Location Config. is not ready, Please wait!!!... "<< std::endl;
        managerStatus = prom.get_future().get();
    }
   ~~~~~~

### 5. Exit the application, if SDK is unable to initialize Location Config. ###

   ~~~~~~{.cpp}
   if (managerStatus == ServiceStatus::SERVICE_AVAILABLE) {
        std::cout<< "Location Config. is ready" << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize Location Config."<< std::endl;
        return -1;
    }
   ~~~~~~

### 6. Enable/Disable Constraint Tunc API ###

   ~~~~~~{.cpp}
    locConfigurator_->configureCTunc(enable, CmdResponse, optThreshold, optPower);
   ~~~~~~

### 7. Command response callback is invoked with error code indicating SUCCESS or FAILURE of the operation. ###
