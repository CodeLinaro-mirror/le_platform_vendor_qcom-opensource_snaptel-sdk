cmake_minimum_required(VERSION 3.10.2)

if (TELSDK_FEATURE_SATCOM)
    add_subdirectory( satcom )
endif(TELSDK_FEATURE_SATCOM)
