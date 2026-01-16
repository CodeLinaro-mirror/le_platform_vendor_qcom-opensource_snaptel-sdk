/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SENSOR_REPORT_SERVER_HPP
#define SENSOR_REPORT_SERVER_HPP

#include "event/EventServiceHelper.hpp"
#include "protos/proto-src/sensor_simulation.grpc.pb.h"

class SensorReportService final : public EventServiceHelper<::sensorStub::EventDispatcherService> {
    /**
     * @brief This class acts as the report event service for the sensor framework
     *        on the server side & is responsible for forwarding the reports
     *        to the eventManager on the client side via writing to the stream.
     */
 public:
    static SensorReportService &getInstance();

 private:
    SensorReportService();
    ~SensorReportService();
};

#endif  // SENSOR_REPORT_SERVER_HPP