cmake_minimum_required(VERSION 2.8.9)

pkg_check_modules(PWR_MGR_QMI_LIB powermanagerqmi)
if((NOT BUILD_FOR_SIMULATION) AND (PWR_MGR_QMI_LIB_FOUND))
    add_subdirectory( tcuactivity_sample_app )
endif((NOT BUILD_FOR_SIMULATION) AND (PWR_MGR_QMI_LIB_FOUND))
