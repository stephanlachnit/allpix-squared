/**
 * @file
 * @brief Definition of MuPix10 for the MuPix digitizer module
 * @copyright Copyright (c) 2021 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include "MuPix10.hpp"

#include <numeric>

#include "tools/ROOT.h"

using namespace allpix::mupix;

MuPix10::MuPix10(Configuration& config) : MuPixModel(config) {
    // Set default parameters
    config.setDefaultArray<double>(
        "parameters",
        {Units::get(2.51424577e+14, "V/C"), Units::get(3.35573247e-07, "s"), Units::get(1.85969061e+04, "V/s")});

    // Get parameters
    auto parameters = config.getArray<double>("parameters");

    // Check that there are exactly three parameters
    if(parameters.size() != 3) {
        throw InvalidValueError(
            config,
            "parameters",
            "The number of parameters does not line up with the amount of parameters for the MuPix10 (3).");
    }

    A_ = parameters[0];
    R_ = parameters[1];
    F_ = parameters[2];
}

double MuPix10::impulse_response_function(double time, double charge) const {
    double out = charge * A_ * (1 - exp(-time / R_)) - F_ * time;
    return (out < 0. ? 0. : out);
}
