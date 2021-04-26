/**
 * @file
 * @brief Definition of the common MuPixModel class
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_MUPIX_CHIP_H
#define ALLPIX_MUPIX_CHIP_H

#include <tuple>
#include <vector>

#include "core/config/Configuration.hpp"

namespace allpix {
    namespace mupix {
        /**
         * @brief Reference Implementation for MuPix type digitization
         *
         * This class provides three main reference functions: amplification,
         * TS1 and TS2 calculation. It also provides a virtual impulse response
         * function that needs to implemented if one wants to use the reference
         * amplification code.
         */
        class MuPixModel {
        public:
            /**
             * @brief Constructor for this base mupix model
             * @param config Configuration object as retrieved from the module
             */
            MuPixModel(Configuration& config);

            /**
             * @brief Destructor for this class
             */
            virtual ~MuPixModel() = default;

            /**
             * @brief Amplifies charge pulse
             * @param timestep Step size of the input pulse
             * @param pulse    Charge pulse
             * @return Amplified pulse
             */
            virtual std::vector<double> amplify_pulse(double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Calculate time of first threshold crossing (TS1)
             * @param timestep Step size of the input pulse
             * @param pulse    Pulse after amplification and electronics noise
             * @return Tuple containing information if crossed  and TS1 cycles of crossing
             */
            virtual std::tuple<bool, unsigned int> get_ts1(double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Calculate time of last threshold crossing from above to below (TS2)
             * @param ts1      TS1 clock cycles of arrival
             * @param timestep Step size of the input pulse
             * @param pulse    Pulse after amplification and electronics noise
             * @return TS2 cycles of crossing
             */
            virtual unsigned int get_ts2(unsigned int ts1, double timestep, const std::vector<double>& pulse) const;

            /**
             * @brief Returns the TS1 clock bin size
             * @return TS1 clock bin size
             */
            double get_ts1_clock() const { return ts1_clock_; };

            /**
             * @brief Returns the TS2 clock bin size
             * @return TS2 clock bin size
             */
            double get_ts2_clock() const { return ts2_clock_; };

            /**
             * @brief Returns the pulse integration time
             * @return integration time
             */
            double get_integration_time() const { return integration_time_; };

            /**
             * @brief Returns the TS2 integration time
             * @return TS2 integration time
             */
            double get_ts2_integration_time() const { return ts2_integration_time_; };

        protected:
            /**
             * @brief Calculate the impulse response for a given time with a given charge.
             * @note This needs to be implemented if the reference amplification code is used.
             * @param time   time after the initial response
             * @param charge charge at the initial response
             * @return voltage of the response
             */
            virtual double impulse_response_function(double time, double charge) const;

            // Whether to amplify the pulse fast or precise
            bool fast_amplification_{};

            // Parameters for the threshold logic
            double threshold_{}, ts1_clock_{}, ts2_clock_{};

            // Pulse calculation times
            double integration_time_{}, ts2_integration_time_{};
        };
    } // namespace mupix
} // namespace allpix

#endif /* ALLPIX_MUPIX_CHIP_H */
