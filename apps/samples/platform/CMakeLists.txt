cmake_minimum_required(VERSION 3.10.2)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( efs_backup_restore_app )
    add_subdirectory( ota_operations_app )
    add_subdirectory( ecall_operations_app )
endif(NOT BUILD_FOR_SIMULATION)
