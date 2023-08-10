export SIM_REPO := ${PWD}
export ROOTFS_BN := $(shell basename ${ROOTFS})

all: apps

rootfs: grpc jsoncpp all

setup: cmake grpc jsoncpp

clean:
	rm -rf build/build_telux_headers build/build_sim build/build_apps

cleanall:
	rm -rf build simulation/protos/proto-src

headers:
	cd build && mkdir -p build_telux_headers && cd build_telux_headers && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../include && make install

jsoncpp:
	cd build && if [ ! -d jsoncpp ] ; then git clone https://git.codelinaro.org/clo/le/jsoncpp.git jsoncpp ; fi && mkdir -p build_jsoncpp && cd build_jsoncpp && cmake -DCMAKE_INSTALL_PREFIX=${ROOTFS} -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_INCLUDEDIR=${ROOTFS}/include/jsoncpp ../jsoncpp && make install
	mv ${ROOTFS}/lib/x86_64-linux-gnu/libjson* ${ROOTFS}/lib/ && rm -rf ${ROOTFS}/lib/x86_64-linux-gnu

cmake:
	mkdir build && cd build && wget https://cmake.org/files/v3.15/cmake-3.15.3.tar.gz && tar -zxvf cmake-3.15.3.tar.gz && rm cmake-3.15.3.tar.gz && cd cmake-3.15.3 && ./configure --prefix=${PWD}/build/ && make -j16 && make install

grpc:
	cd build && if [ ! -d grpc_repo ] ; then git clone --recurse-submodules -b v1.54.0 --depth 1 --shallow-submodules https://git.codelinaro.org/clo/le/grpc_repo.git ; fi && mkdir -p build_grpc && cd build_grpc && ${PWD}/build/bin/cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DgRPC_INSTALL_SHAREDIR=${PWD}/build/ -DgRPC_INSTALL_INCLUDEDIR=${PWD}/build/include/ -DCMAKE_INSTALL_INCLUDEDIR=${PWD}/build/include/ -DgRPC_INSTALL_BINDIR=${PWD}/build/bin/ -DCMAKE_INSTALL_BINDIR=${PWD}/build/bin/ -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../grpc_repo && make -j16 && make install
	cp -r ${PWD}/build/include/google ${ROOTFS}/include/ &&	cp -r ${PWD}/build/include/grpc ${ROOTFS}/include/ && cp -r ${PWD}/build/include/grpcpp ${ROOTFS}/include/ && cp -r ${PWD}/build/include/absl ${ROOTFS}/include/

sim: headers
	cd build && export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${ROOTFS}/lib/ && mkdir -p build_sim && cd build_sim && cmake -DCMAKE_VERBOSE_MAKEFILE=ON -DCMAKE_PREFIX_PATH=${PWD}/build/lib/cmake/ -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../simulation && make install

apps: sim
	cd build && mkdir -p build_apps && cd build_apps && cmake -DCMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES=${ROOTFS}/include -DCMAKE_INSTALL_PREFIX=${ROOTFS} ../../apps && make install
	mkdir -p ${ROOTFS}/etc/telux/ && mkdir -p ${ROOTFS}/etc/telux/json/ && cp -r simulation/json/* ${ROOTFS}/etc/telux/json/ && cp simulation/conf/tel.conf ${ROOTFS}/etc/telux/ && cp simulation/setup_simualtion.sh ${ROOTFS}/bin/ && mkdir -p ${ROOTFS}/etc/init.d && cp simulation/conf/telsdk_simulation_server ${ROOTFS}/etc/init.d/

docker-image: apps
	cd ${ROOTFS}/.. && docker build --build-arg="ROOTFS=${ROOTFS_BN}" -t telsdk-sim-image -f ${SIM_REPO}/simulation/Dockerfile . && echo "Docker image "telsdk-sim-image" is created. Use below command to drop to the shell" && echo "docker run -ti --rm -h telsdk_simulation telsdk-sim-image"
