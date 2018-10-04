# How to get data profile list

Please follow below steps to request list of available modem profiles

### 1. Get the DataFactory and DataProfileManager instances

   ~~~~~~{.cpp}
   auto &dataFactory = DataFactory::getInstance();
   auto dataProfileMgr = dataFactory.getDataProfileManager();
   ~~~~~~

### 2. Instantiate requestProfileList callback ###

   ~~~~~~{.cpp}
   auto dataProfileListCb_ = std::make_shared<DataProfileListCallback>();
   ~~~~~~

### 2.1 Implement IDataProfileListCallback interface to know status of requestProfileList ###

   ~~~~~~{.cpp}
   class DataProfileListCallback : public telux::common::IDataProfileListCallback {

     virtual void onProfileListResponse(const std::vector<std::shared_ptr<DataProfile>> &profiles,
                                        telux::common::ErrorCode error) override {

      std::cout<<"Length of available profiles are "<<profiles.size()<<std::endl;
     }
   };
   ~~~~~~

### 3. Send a requestProfileList along with required callback function ###

   ~~~~~~{.cpp}
   telux::common::Status status = dataProfileMgr->requestProfileList(dataProfileListCb_);
   ~~~~~~

### 4. Receive DataProfileListCallback responses for requestProfileList request ###