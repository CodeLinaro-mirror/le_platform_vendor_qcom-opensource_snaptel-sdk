/*
 * Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       Cv2xManagerServerImpl.cpp
 *
 *
 */

#include "Cv2xManagerServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"

#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

static const std::string RADIO_ROOT = "ICv2xRadio";
static const std::string RADIO_STATE_JSON = "system-state/cv2x/ICv2xRadio.json";

static const std::string CV2X_MGR_API_JSON = "api/cv2x/ICv2xManager.json";
static const std::string CV2X_MGR_NODE = "ICv2xManager";

static const std::string CV2X_EVENT_FILTER = "cv2x_status";

telux::common::ErrorCode
Cv2xServerUtil::stateJasonRead(std::string stateCfgFile, Json::Value &data) {
  telux::common::ErrorCode err =
      JsonParser::readFromJsonFile(data, stateCfgFile);
  if (err != ErrorCode::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Reading JSON File ", stateCfgFile, " failed! ");
    return err;
  }

  return err;
}

cv2xStub::Cv2xStatus_StatusType Cv2xServerUtil::strToStatus(std::string str) {
  if (str.compare("inactive") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_INACTIVE;
  } else if (str.compare("active") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_ACTIVE;
  } else if (str.compare("suspended") == 0) {
    return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_SUSPENDED;
  }
  return cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_Status_UNKNOWN;
}

Cv2xServerEvtListener::Cv2xServerEvtListener() {
  LOG(DEBUG, __FUNCTION__);
  readDefaultStatus();
}

void Cv2xServerEvtListener::readDefaultStatus() {
  std::string method = "cv2xDefaultStatus";
  Json::Value data;

  if (telux::common::ErrorCode::SUCCESS ==
      Cv2xServerUtil::stateJasonRead(RADIO_STATE_JSON, data)) {
    stubStatus_.set_rxstatus(Cv2xServerUtil::strToStatus(
        data[RADIO_ROOT][method]["rxStatus"].asString()));
    stubStatus_.set_txstatus(Cv2xServerUtil::strToStatus(
        data[RADIO_ROOT][method]["txStatus"].asString()));

    try {
      auto num = data[RADIO_ROOT][method]["rxCause"].asInt();
      stubStatus_.set_rxcause(static_cast<::cv2xStub::Cv2xStatus_Cause>(num));
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return;
    }

    try {
      auto num = data[RADIO_ROOT][method]["txCause"].asInt();
      stubStatus_.set_txcause(static_cast<::cv2xStub::Cv2xStatus_Cause>(num));
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return;
    }
  }
}

const shared_ptr<Cv2xServerEvtListener> &Cv2xServerEvtListener::getInstance() {
  static shared_ptr<Cv2xServerEvtListener> instance(new Cv2xServerEvtListener);
  return instance;
}

bool Cv2xServerEvtListener::stringToStatus(std::string &str,
                                           cv2xStub::Cv2xStatus &result,
                                           bool &needParseCause, bool rx) {
  std::string strToken = EventParserUtil::getNextToken(str, " ");
  cv2xStub::Cv2xStatus_StatusType token;

  if (not strToken.empty()) {
    try {
      token = Cv2xServerUtil::strToStatus(strToken);
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return false;
    }
  } else {
    LOG(DEBUG, __FUNCTION__, " strToken is empty.");
    return false;
  }

  if (cv2xStub::Cv2xStatus_StatusType_IsValid(static_cast<int>(token))) {
    if (rx) {
      result.set_rxstatus(token);
    } else {
      result.set_txstatus(token);
    }
    if (cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_INACTIVE ==
            token ||
        cv2xStub::Cv2xStatus_StatusType::Cv2xStatus_StatusType_SUSPENDED ==
            token) {
      needParseCause = true;
    }
    return true;
  } else {
    LOG(DEBUG, __FUNCTION__, " StatusTyp is not in range ",
        static_cast<int>(token));
  }
  return false;
}

bool Cv2xServerEvtListener::stringToCause(std::string &str,
                                          cv2xStub::Cv2xStatus &result,
                                          bool rx) {
  std::string strToken = EventParserUtil::getNextToken(str, " ");
  int token;

  if (not strToken.empty()) {
    try {
      token = std::stoi(strToken);
    } catch (exception const &ex) {
      LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
      return false;
    }
  } else {
    LOG(DEBUG, __FUNCTION__, " strToken is empty.");
    return false;
  }

  if (cv2xStub::Cv2xStatus_Cause_IsValid(token)) {
    if (rx) {
      result.set_rxcause(static_cast<cv2xStub::Cv2xStatus_Cause>(token));
    } else {
      result.set_txcause(static_cast<cv2xStub::Cv2xStatus_Cause>(token));
    }
    return true;
  } else {
    LOG(DEBUG, __FUNCTION__, " causecode is not in range ", token);
  }
  return false;
}

void Cv2xServerEvtListener::OnCv2xStatusChange(std::string str) {
  bool update = false;
  bool needRxCause = false;
  bool needTxCause = false;
  cv2xStub::Cv2xStatus tmpStatus;
  LOG(DEBUG, __FUNCTION__, str);

  do {
    if (not stringToStatus(str, tmpStatus, needRxCause, true)) {
      break;
    }
    if (not stringToStatus(str, tmpStatus, needTxCause, false)) {
      break;
    }

    if (needRxCause) {
      if (not stringToCause(str, tmpStatus, true)) {
        break;
      }
    }
    if (needTxCause) {
      if (not stringToCause(str, tmpStatus, false)) {
        break;
      }
    }
    update = true;
  } while (0);

  if (update) {
    {
      std::lock_guard<std::mutex> lock(statusMtx_);
      stubStatus_ = tmpStatus;
    }
    notifyListeners(tmpStatus);
    ::eventService::EventResponse statusChangeInd;
    statusChangeInd.set_filter(CV2X_EVENT_FILTER);
    statusChangeInd.mutable_any()->PackFrom(tmpStatus);
    // posting the event to EventService event queue
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(statusChangeInd);
  } else {
    LOG(INFO, __FUNCTION__, " Cv2x status assume no change");
  }
}

void Cv2xServerEvtListener::onEventUpdate(
    ::eventService::UnsolicitedEvent message) {
  LOG(DEBUG, __FUNCTION__, message.filter());
  if (message.filter() == CV2X_EVENT_FILTER) {
    auto event = message.event();
    LOG(DEBUG, __FUNCTION__, " message.event ", event);
    if (not event.empty()) {
      OnCv2xStatusChange(event);
    }
  }
}

cv2xStub::Cv2xStatus Cv2xServerEvtListener::getCv2xStatus() {
  std::lock_guard<std::mutex> lock(statusMtx_);
  return stubStatus_;
}

void Cv2xServerEvtListener::notifyListeners(cv2xStub::Cv2xStatus &stub) {
  std::vector<std::weak_ptr<telux::cv2x::ICv2xListener>> lists;
  telux::cv2x::Cv2xStatus status;

  status.rxStatus = static_cast<telux::cv2x::Cv2xStatusType>(stub.rxstatus());
  status.txStatus = static_cast<telux::cv2x::Cv2xStatusType>(stub.txstatus());
  status.rxCause = static_cast<telux::cv2x::Cv2xCauseType>(stub.rxcause());
  status.txCause = static_cast<telux::cv2x::Cv2xCauseType>(stub.txcause());

  listenerMgr_.getAvailableListeners(lists);
  for (auto &wp : lists) {
    if (auto sp = wp.lock()) {
      sp->onStatusChanged(status);
    }
  }
}

telux::common::Status Cv2xServerEvtListener::registerListener(
    std::weak_ptr<telux::cv2x::ICv2xListener> l) {
  return listenerMgr_.registerListener(l);
}

telux::common::Status Cv2xServerEvtListener::deregisterListener(
    std::weak_ptr<telux::cv2x::ICv2xListener> l) {
  return listenerMgr_.deRegisterListener(l);
}

Cv2xManagerServerImpl::Cv2xManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  evtListener_ = Cv2xServerEvtListener::getInstance();
  if (evtListener_) {
    std::vector<std::string> filters = {CV2X_EVENT_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(evtListener_, filters);
  }
}

Cv2xManagerServerImpl::~Cv2xManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  if (evtListener_) {
    std::vector<std::string> filters = {CV2X_EVENT_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.deregisterListener(evtListener_, filters);
  }
}

grpc::Status
Cv2xManagerServerImpl::initService(ServerContext *context,
                                   const google::protobuf::Empty *request,
                                   cv2xStub::GetServiceStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  int cbDelay = 100;
  telux::common::ServiceStatus serviceStatus =
      telux::common::ServiceStatus::SERVICE_FAILED;
  Json::Value rootNode;
  telux::common::ErrorCode errorCode =
      JsonParser::readFromJsonFile(rootNode, CV2X_MGR_API_JSON);
  if (errorCode == ErrorCode::SUCCESS) {
    cbDelay = rootNode[CV2X_MGR_NODE]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootNode[CV2X_MGR_NODE]["IsSubsystemReady"].asString();
    serviceStatus = CommonUtils::mapServiceStatus(cbStatus);
  } else {
    LOG(ERROR, "Unable to read Cv2xManager JSON");
  }
  res->set_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
  res->set_delay(cbDelay);
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::startCv2x(ServerContext *context,
                                 const google::protobuf::Empty *request,
                                 cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE, "startCv2x",
                                res);
  evtListener_->readDefaultStatus();
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::stopCv2x(ServerContext *context,
                                const google::protobuf::Empty *request,
                                cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE, "stopCv2x",
                                res);

  evtListener_->OnCv2xStatusChange("inactive inactive 2 2");
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::setPeakTxPower(ServerContext *context,
                                      const cv2xStub::Cv2xPeakTxPower *request,
                                      ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "setPeakTxPower", res);
  return grpc::Status::OK;
}

grpc::Status Cv2xManagerServerImpl::injectCoarseUtcTime(
    ServerContext *context, const ::cv2xStub::CoarseUtcTime *request,
    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__, " utc: ", request->utc());
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "injectCoarseUtcTime", res);
  return grpc::Status::OK;
}

grpc::Status Cv2xManagerServerImpl::requestCv2xStatus(
    ServerContext *context, const google::protobuf::Empty *request,
    ::cv2xStub::Cv2xRequestStatusReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "requestCv2xStatus", res);
  if (evtListener_) {
    ::cv2xStub::Cv2xStatus *resp = res->mutable_cv2xstatus();
    if (resp) {
      *resp = evtListener_->getCv2xStatus();
    }
  }
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::getSlssRxInfo(ServerContext *context,
                                     const google::protobuf::Empty *request,
                                     ::cv2xStub::SlssRxInfoReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "getSlssRxInfo", res);

  Json::Value rootNode;
  uint32_t slssId = 1;
  bool inCoverage = true;
  telux::cv2x::SlssSyncPattern pattern = telux::cv2x::SlssSyncPattern::OFFSET_IND_1;
  uint32_t rsrp = 1;
  bool selected = true;

  ::cv2xStub::SyncRefUeInfo *ueInfo = res->add_info();

  ueInfo->set_slssid(slssId);
  ueInfo->set_incoverage(inCoverage);
  ueInfo->set_pattern(
      static_cast<::cv2xStub::SyncRefUeInfo::SlssSyncPattern>(pattern));
  ueInfo->set_rsrp(rsrp);
  ueInfo->set_selected(selected);

  LOG(DEBUG, __FUNCTION__, " slssId: ", ueInfo->slssid(),
      " inCoverage: ", ueInfo->incoverage());

  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::setL2Filters(ServerContext *context,
                                    const ::cv2xStub::L2FilterInfos *request,
                                    ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "setL2Filters", res);
  return grpc::Status::OK;
}

grpc::Status
Cv2xManagerServerImpl::removeL2Filters(ServerContext *context,
                                       const ::cv2xStub::L2Ids *request,
                                       ::cv2xStub::Cv2xCommandReply *res) {
  LOG(DEBUG, __FUNCTION__);
  Cv2xServerUtil::apiJsonReader(CV2X_MGR_API_JSON, CV2X_MGR_NODE,
                                "removeL2Filters", res);
  return grpc::Status::OK;
}
