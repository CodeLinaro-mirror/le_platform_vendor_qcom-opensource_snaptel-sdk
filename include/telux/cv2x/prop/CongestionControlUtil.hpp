/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_CV2X_PROP_CONGESTIONCONTROLUTIL_HPP
#define TELUX_CV2X_PROP_CONGESTIONCONTROLUTIL_HPP

namespace telux {
    namespace cv2x {
        namespace prop {

        /** @addtogroup telematics_cv2x_cpp
         * @{ */

        class CongestionControlUtil {
        public:

        /**
         * Return current time stamp in seconds
         * @returns long long
         *
         */
        uint64_t timestamp_now(void);

        /**
         * Return current time stamp in milliseconds
         * @returns long long
         *
         */
        uint64_t timestamp_now_ms(void);

        /**
         * Return current time stamp in nanoseconds
         * @returns long long
         *
         */
        uint64_t timestamp_now_ns(void);

        };

        /** @} */ /* end_addtogroup telematics_cv2x_cpp */

        }  // End of namespace prop
    }  // End of namespace cv2x
}  // End of namespace telux
#endif // TELUX_CV2X_PROP_CONGESTIONCONTROLUTIL_HPP
