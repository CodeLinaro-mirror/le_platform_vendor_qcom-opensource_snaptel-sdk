# Enable L2TP and Add Tunnel

Please follow below steps to enable L2TP and Tunnel

### 1. Get the DataFactory and L2tp Manager instance

   ~~~~~~{.cpp}
   auto &dataFactory = telux::data::DataFactory::getInstance();
   auto dataL2tpMgr  = dataFactory.getL2tpManager();
   ~~~~~~

### 2. Check if data subsystem is ready

   ~~~~~~{.cpp}
   bool subSystemStatus = dataL2tpMgr->isSubsystemReady();
   ~~~~~~

### 2.1 If data subsystem is not ready, wait for it to be ready

   ~~~~~~{.cpp}
   if(!subSystemStatus) {
      std::future<bool> f = dataL2tpMgr->onSubsystemReady();
      subSystemStatus = f.get();
   }
   ~~~~~~

### 3. Instantiate setConfig callback instance - this is optional

   ~~~~~~{.cpp}
   auto setConfigCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   std::cout << "CALLBACK: "
             << "setConfig Response"
             << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed");
   };
   ~~~~~~

### 4. Set L2TP Configuration
   ~~~~~~{.cpp}
   bool enable = true;        //Enable L2TP
   bool enableMss = true;     // Enable MSS Clamping
   bool enableMtu = true;     // Enable custom size MTU
   int mtuSize = 0;           // Set MTU size to default 1422 bytes, otherwise set desired mtu size
   dataL2tpMgr->setConfig(enable, enableMss, enableMtu, setConfigCb, mtuSize);
   ~~~~~~

### 5. Configure L2TP Tunnel and Session

   ~~~~~~{.cpp}
   std::cout << "L2TP Set Configuration succeeded ... Adding Tunnel" << std::endl;
   telux::data::net::L2tpTunnelConfig l2tpTunnelConfig;
   l2tpTunnelConfig.locIface = "eth0.1"; //Set interface name to eth0.x where x is vlan id
   l2tpTunnelConfig.prot = static_cast<telux::data::net::L2tpProtocol>(2); //Set protocol to UDP
   l2tpTunnelConfig.locId = 1;  //Set local tunnel id
   l2tpTunnelConfig.peerId = 1; //Set peer tunnel id
   l2tpTunnelConfig.localUdpPort = 500;   //Set local UDP port if UDP protocol is selected above
   l2tpTunnelConfig.peerUdpPort = 100;    //Set peer UDP port if UDP protocol is selected above
   l2tpTunnelConfig.ipType =  static_cast<telux::data::IpFamilyType>(6); //Set Ip family type
   l2tpTunnelConfig.peerIpv6Addr =  "fe80::b044::c0ff::fec4";  // Set peer Ip address
   telux::data::net::L2tpSessionConfig l2tpSessionConfig;
   l2tpSessionConfig.locId = 1;   //Set local session id
   l2tpSessionConfig.peerId = 1;  //Set peer session id
   l2tpTunnelConfig.sessionConfig.emplace_back(l2tpSessionConfig); // Add session to tunnel config
   ~~~~~~

###   6. Instantiate addTunnel callback instance - this is optional

   ~~~~~~{.cpp}
   auto addTunnelCb = [&setConfigPass, &promise](telux::common::ErrorCode error) {
      std::cout << std::endl << std::endl;
      std::cout << "CALLBACK: "
                << "addTunnel Response"
                << (error == telux::common::ErrorCode::SUCCESS ? " is successful" : " failed")
                << ". ErrorCode: " << static_cast<int>(error) << "\n";
   };
   ~~~~~~

### 7. addTunnel to L2TP

   ~~~~~~{.cpp}
   dataL2tpMgr->addTunnel(l2tpTunnelConfig, addTunnelCb);
   ~~~~~~
