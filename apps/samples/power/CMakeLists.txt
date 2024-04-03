cmake_minimum_required(VERSION 3.10.2)

pkg_check_modules(PWR_MGR_QMI_LIB powermanagerqmi)
if((NOT BUILD_FOR_SIMULATION) AND (PWR_MGR_QMI_LIB_FOUND))
    add_subdirectory(tcuactivity_slave)
    add_subdirectory(tcuactivity_master)
    add_subdirectory(mpss_power_save)
endif((NOT BUILD_FOR_SIMULATION) AND (PWR_MGR_QMI_LIB_FOUND))
