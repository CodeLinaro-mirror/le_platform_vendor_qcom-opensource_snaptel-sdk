all: headers sim apps

setup: cmake grpc jsoncpp

clean:
	rm -rf build_telux_headers build_sim build_apps

cleanall: clean
	rm -rf build build_jsoncpp build_grpc jsoncpp grpc_repo cmake-3.15.3

headers:
	mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../include && make install

jsoncpp:
	git clone https://git.codelinaro.org/clo/le/jsoncpp.git jsoncpp && mkdir -p build_jsoncpp && cd build_jsoncpp && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_INCLUDEDIR=${ROOTFS}/include/jsoncpp ../jsoncpp && make install
	mv ${ROOTFS}/lib/x86_64-linux-gnu/libjson* ${ROOTFS}/lib/ && rm -rf ${ROOTFS}/lib/x86_64-linux-gnu

cmake:
	wget https://cmake.org/files/v3.15/cmake-3.15.3.tar.gz && tar -zxvf cmake-3.15.3.tar.gz && rm cmake-3.15.3.tar.gz && cd cmake-3.15.3 && ./configure --prefix=${PWD}/build && make -j16 && make install && export PATH=${PWD}/build/bin/:$PATH

grpc:
	git clone --recurse-submodules -b v1.55.0 --depth 1 --shallow-submodules https://git.codelinaro.org/clo/le/grpc_repo.git && mkdir -p build_grpc && cd build_grpc && ${PWD}/build/bin/cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL_SHAREDIR=${PWD}/build/ -DgRPC_INSTALL_INCLUDEDIR=${PWD}/build/include/ -DCMAKE_INSTALL_INCLUDEDIR=${PWD}/build/include/ -DgRPC_INSTALL_BINDIR=${PWD}/build/bin/ -DCMAKE_INSTALL_BINDIR=${PWD}/build/bin/ -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../grpc_repo && make -j16 && make install
	cp -r ${PWD}/build/include/google ${ROOTFS}/include/ &&	cp -r ${PWD}/build/include/grpc ${ROOTFS}/include/ && cp -r ${PWD}/build/include/grpcpp ${ROOTFS}/include/ && cp -r ${PWD}/build/include/absl ${ROOTFS}/include/

sim: headers
	export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTFS}/lib/ && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH=${PWD}/build/lib/cmake/ -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../simulation && make install

apps: sim
	mkdir -p build_apps && cd build_apps && cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../apps && make install
	mkdir -p ${ROOTFS}/etc/telux/conf/ && mkdir -p ${ROOTFS}/etc/telux/json/ && cp -r ${PWD}/json/* ${ROOTFS}/etc/telux/json/ && cp ${PWD}/conf/simulation.conf ${ROOTFS}/etc/telux/conf/