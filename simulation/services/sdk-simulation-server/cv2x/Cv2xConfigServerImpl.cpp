/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xConfigServerImpl.cpp
 *
 *
 */
#include "Cv2xConfigServerImpl.hpp"
#include "Cv2xHelperServer.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/cv2x/Cv2xRadioHelperStub.hpp"

static const std::string CV2X_CFG_API_JSON = "api/cv2x/ICv2xConfig.json";
static const std::string CV2X_CFG_ROOT = "ICv2xConfig";

Cv2xConfigServerImpl::Cv2xConfigServerImpl() { LOG(DEBUG, __FUNCTION__); }

Cv2xConfigServerImpl::~Cv2xConfigServerImpl() { LOG(DEBUG, __FUNCTION__); }

grpc::Status
Cv2xConfigServerImpl::initService(ServerContext *context,
                                  const google::protobuf::Empty *request,
                                  cv2xStub::GetServiceStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  int cbDelay = DEFAULT_DELAY;
  telux::common::ServiceStatus serviceStatus =
      telux::common::ServiceStatus::SERVICE_FAILED;
  Json::Value rootNode;
  telux::common::ErrorCode errorCode =
      JsonParser::readFromJsonFile(rootNode, CV2X_CFG_API_JSON);
  if (errorCode == ErrorCode::SUCCESS) {
    cbDelay = rootNode[CV2X_CFG_ROOT]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootNode["ICv2xConfig"]["IsSubsystemReady"].asString();
    serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
  } else {
    LOG(ERROR, "Unable to read Cv2xConfig JSON");
  }
  res->set_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
  res->set_delay(cbDelay);
  return grpc::Status::OK;
}

grpc::Status Cv2xConfigServerImpl::updateConfiguration(
    ServerContext *context, const cv2xStub::Cv2xConfigPath *request,
    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__, " cv2x config path: ", request->path());

  Cv2xServerUtil::apiJsonReader(CV2X_CFG_API_JSON, CV2X_CFG_ROOT,
                                "updateConfiguration", res);

  path_ = request->path();
  return grpc::Status::OK;
}

grpc::Status Cv2xConfigServerImpl::retrieveConfiguration(
    ServerContext *context, const cv2xStub::Cv2xConfigPath *request,
    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__, " cv2x config path: ", request->path());

  Cv2xServerUtil::apiJsonReader(CV2X_CFG_API_JSON, CV2X_CFG_ROOT,
                                "retrieveConfiguration", res);

  return grpc::Status::OK;
}
