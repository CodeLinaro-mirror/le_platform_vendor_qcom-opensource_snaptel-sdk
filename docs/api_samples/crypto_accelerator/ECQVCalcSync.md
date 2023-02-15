Calculate ECQV point synchronously {#ecqv_calc_sync}
====================================================

This sample app demonstrates how to calculate ECQV point synchronously. The
steps are:

1. Get SecurityFactory instance.
2. Get ICryptoAcceleratorManager instance from SecurityFactory.
3. Define parameters for calculation process.
4. Send parameters for calculation (method will block here).
   When the calculation is complete, get the result.

   ~~~~~~{.cpp}
#include <iostream>

#include <telux/sec/SecurityFactory.hpp>

/* Scalar (hash construct) */
uint8_t scl[] = {
  /* 39c9a40f5b60bb6131ffe4123e6b6f29b451e7031f7b59fffe0a65bf4e3b07d1 */
  0x4e, 0x3b, 0x07, 0xd1, 0xfe, 0x0a, 0x65, 0xbf, 0x1f, 0x7b, 0x59, 0xff, 0xb4, 0x51,
  0xe7, 0x03, 0x3e, 0x6b, 0x6f, 0x29, 0x31, 0xff, 0xe4, 0x12, 0x5b, 0x60, 0xbb, 0x61,
  0x39, 0xc9, 0xa4, 0x0f };

/* Point to multiply (public key reconstruction value) */
uint8_t mulPointX[] = {
  /* a67d0283cee35d32fc4a21e0d87b7eb872c2d175dde0e517cc25d8c14211b379 */
  0x42, 0x11, 0xb3, 0x79, 0xcc, 0x25, 0xd8, 0xc1, 0xdd, 0xe0, 0xe5, 0x17, 0x72, 0xc2,
  0xd1, 0x75, 0xd8, 0x7b, 0x7e, 0xb8, 0xfc, 0x4a, 0x21, 0xe0, 0xce, 0xe3, 0x5d, 0x32,
  0xa6, 0x7d, 0x02, 0x83 };
uint8_t mulPointY[] = {
  /* a5a907937eb58ff6b03dded1c6baa68fbc0316b7bd21a6bc91b59e7c759396a5 */
  0x75, 0x93, 0x96, 0xa5, 0x91, 0xb5, 0x9e, 0x7c, 0xbd, 0x21, 0xa6, 0xbc, 0xbc, 0x03,
  0x16, 0xb7, 0xc6, 0xba, 0xa6, 0x8f, 0xb0, 0x3d, 0xde, 0xd1, 0x7e, 0xb5, 0x8f, 0xf6,
  0xa5, 0xa9, 0x07, 0x93};

/* Point to add (CA public key) */
uint8_t addPointX[] = {
  /* e96d941f1b92ce904d937b1180b6b5684c7b1332be9b79c2b4eab667b140485c */
  0xb1, 0x40, 0x48, 0x5c, 0xb4, 0xea, 0xb6, 0x67, 0xbe, 0x9b, 0x79, 0xc2, 0x4c, 0x7b,
  0x13, 0x32, 0x80, 0xb6, 0xb5, 0x68, 0x4d, 0x93, 0x7b, 0x11, 0x1b, 0x92, 0xce, 0x90,
  0xe9, 0x6d, 0x94, 0x1f };
uint8_t addPointY[] = {
  /* 40ca3c9f4c9d1cd617d798565f597b5b0ca35529407d5a344ab235cbe784b16f */
  0xe7, 0x84, 0xb1, 0x6f, 0x4a, 0xb2, 0x35, 0xcb, 0x40, 0x7d, 0x5a, 0x34, 0x0c, 0xa3,
  0x55, 0x29, 0x5f, 0x59, 0x7b, 0x5b, 0x17, 0xd7, 0x98, 0x56, 0x4c, 0x9d, 0x1c, 0xd6,
  0x40, 0xca, 0x3c, 0x9f };

/* Expected result of the calculation */
uint8_t outPointX[] = {
  /* 9b2bc64ed3c6275c63dc965268a4f90b7b2c47e48e9531e68be0f0b76930faa8 */
  0x69, 0x30, 0xfa, 0xa8, 0x8b, 0xe0, 0xf0, 0xb7, 0x8e, 0x95, 0x31, 0xe6, 0x7b, 0x2c,
  0x47, 0xe4, 0x68, 0xa4, 0xf9, 0x0b, 0x63, 0xdc, 0x96, 0x52, 0xd3, 0xc6, 0x27, 0x5c,
  0x9b, 0x2b, 0xc6, 0x4e };
uint8_t outPointY[] = {
  /* f374625b8097d81ac7ef7e35c18043e34c6541a00aad833fe0ec65a6aa35942d */
  0xaa, 0x35, 0x94, 0x2d, 0xe0, 0xec, 0x65, 0xa6, 0x0a, 0xad, 0x83, 0x3f, 0x4c, 0x65,
  0x41, 0xa0, 0xc1, 0x80, 0x43, 0xe3, 0xc7, 0xef, 0x7e, 0x35, 0x80, 0x97, 0xd8, 0x1a,
  0xf3, 0x74, 0x62, 0x5b };

int main(int argc, char **argv) {

    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    uint32_t uniqueId;
    telux::sec::ECCCurve curve;
    telux::sec::ECCPoint multiplicandPoint{};
    telux::sec::ECCPoint addendPoint{};
    telux::sec::Scalar scalar{};
    telux::sec::RequestPriority priority;

    /* Step - 1 */
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    /* Step - 2 */
    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_SYNC);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err " << static_cast<int>(ec) << std::endl;
        return -ENOMEM;
    }

    /* Step - 3 */
    uniqueId = 1;
    curve = telux::sec::ECCCurve::CURVE_NISTP256;
    priority = telux::sec::RequestPriority::REQ_PRIORITY_NORMAL;
    scalar.scalar = scl;
    scalar.scalarLength = sizeof(scl)/sizeof(scl[0]);
    multiplicandPoint.x = mulPointX;
    multiplicandPoint.xLength = sizeof(mulPointX)/sizeof(mulPointX[0]);
    multiplicandPoint.y = mulPointY;
    multiplicandPoint.yLength = sizeof(mulPointY)/sizeof(mulPointY[0]);
    addendPoint.x = addPointX;
    addendPoint.xLength = sizeof(addPointX)/sizeof(addPointX[0]);
    addendPoint.y = addPointY;
    addendPoint.yLength = sizeof(addPointY)/sizeof(addPointY[0]);

    /* Step - 4 */
    ec = cryptAccelMgr->ecqvPointMultiplyAndAdd(multiplicandPoint, addendPoint,
            scalar, curve, uniqueId, priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, " << static_cast<int>(ec) << std::endl;
        cryptAccelMgr = nullptr;
        return -1;
    }
    std::cout << "calculation done, " << static_cast<int>(ec) << std::endl;

    for (uint32_t x=0; x < resultData.size(); x++) {
        printf("%02x", resultData.at(x) & 0xffU);
        if (x & !(x % 32)) {
            printf("\n");
        }
    }
    printf("\n");

    return 0;
}
   ~~~~~~