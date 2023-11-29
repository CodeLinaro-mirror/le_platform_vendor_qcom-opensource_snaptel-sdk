cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( gen_rn_trng )
    add_subdirectory( gen_rn_dev_random )
endif(NOT BUILD_FOR_SIMULATION)