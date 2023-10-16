cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    tel/CardManagerServerImpl.cpp
    tel/SubscriptionManagerServerImpl.cpp
    tel/SmsManagerServerImpl.cpp
    tel/CallManagerServerImpl.cpp
    tel/EcallStateMachine.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})