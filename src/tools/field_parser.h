/**
 * @file
 * @brief Utility to parse INIT-format field files
 * @copyright Copyright (c) 2017 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#ifndef ALLPIX_FIELD_PARSER_H
#define ALLPIX_FIELD_PARSER_H

#include <fstream>
#include <iostream>
#include <map>

#include "core/utils/log.h"
#include "core/utils/unit.h"

#include <cereal/archives/portable_binary.hpp>

#include <cereal/types/array.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/vector.hpp>

namespace allpix {

    /**
     * @brief Field quantities
     */
    enum class FieldQuantity : size_t {
        UNKNOWN = 0, ///< Unknown fiield quantity
        SCALAR = 1,  ///< Scalar field, i.e. one entry per field position
        VECTOR = 3,  ///< Vector field, i.e. three entries per field position
    };

    /**
     * @brief Type of file formats
     */
    enum class FileType {
        UNKNOWN = 0, ///< Unknown file format
        INIT,        ///< Leagcy file format, values stored in plain-text ASCII
        APF,         ///< Binary Allpix Squared format serialized using the cereal library
    };

    /**
     * Class to hold raw, three-dimensional field data with N components, containing
     * * The actual field data as shared pointer to vector
     * * An array specifying the number of bins in each dimension
     * * An array containing the physical extent of the field in each dimension, as specified in the file
     */
    template <typename T = double> class FieldData {
    public:
        FieldData() = default;
        FieldData(std::string header,
                  std::array<size_t, 3> dimensions,
                  std::array<T, 3> size,
                  std::shared_ptr<std::vector<T>> data)
            : header_(header), dimensions_(dimensions), size_(size), data_(data){};

        std::string getHeader() const { return header_; }
        std::array<size_t, 3> getDimensions() const { return dimensions_; }
        std::array<T, 3> getSize() const { return size_; }
        std::shared_ptr<std::vector<T>> getData() const { return data_; }

    private:
        std::string header_;
        std::array<size_t, 3> dimensions_{};
        std::array<T, 3> size_{};
        std::shared_ptr<std::vector<T>> data_;

        friend class cereal::access;

        template <class Archive> void serialize(Archive& archive) {
            archive(header_);
            archive(dimensions_);
            archive(size_);
            archive(data_);
        }
    };

    template <typename T = double> class FieldParser {
    public:
        FieldParser(const FieldQuantity quantity, const std::string units = std::string()) : units_(std::move(units)) {
            // Store quantity: vector or scalar field:
            N_ = static_cast<std::underlying_type<FieldQuantity>::type>(quantity);
        };
        ~FieldParser() = default;

        /**
         * @brief Get the field from a file name, caching the result
         */
        FieldData<T> get_by_file_name(const std::string& file_name, const FileType& file_type) {
            // Search in cache (NOTE: the path reached here is always a canonical name)
            auto iter = field_map_.find(file_name);
            if(iter != field_map_.end()) {
                LOG(INFO) << "Using cached field data";
                return iter->second;
            }

            switch(file_type) {
            case FileType::INIT:
                return parse_init_file(file_name);
            case FileType::APF:
                return parse_apf_file(file_name);
            default:
                throw std::runtime_error("unknown file format");
            }
        }

    private:
        FieldData<T> parse_apf_file(const std::string& file_name) {
            std::ifstream file(file_name, std::ios::binary);
            FieldData<double> field_data;
            if(!units_.empty()) {
                LOG(WARNING) << "Units will be ignored, APF file content is interpreted in internal units.";
            }

            // Parse the file with cereal, add manual scope to ensure flushing:
            {
                cereal::PortableBinaryInputArchive archive(file);
                archive(field_data);
            }

            // Check that we have the right number of vector entries
            auto dimensions = field_data.getDimensions();
            if(field_data.getData()->size() != dimensions[0] * dimensions[1] * dimensions[2] * N_) {
                throw std::runtime_error("invalid data");
            }

            return field_data;
        }

        FieldData<T> parse_init_file(const std::string& file_name) {
            // Load file
            std::ifstream file(file_name);
            std::string header;
            std::getline(file, header);
            LOG(TRACE) << "Header of file " << file_name << " is " << std::endl << header;

            // Read the header
            std::string tmp;
            file >> tmp >> tmp;        // ignore the init seed and cluster length
            file >> tmp >> tmp >> tmp; // ignore the incident pion direction
            file >> tmp >> tmp >> tmp; // ignore the magnetic field (specify separately)
            double thickness, xpixsz, ypixsz;
            file >> thickness >> xpixsz >> ypixsz;
            thickness = Units::get(thickness, "um");
            xpixsz = Units::get(xpixsz, "um");
            ypixsz = Units::get(ypixsz, "um");
            file >> tmp >> tmp >> tmp >> tmp; // ignore temperature, flux, rhe (?) and new_drde (?)
            size_t xsize, ysize, zsize;
            file >> xsize >> ysize >> zsize;
            file >> tmp;

            if(file.fail()) {
                throw std::runtime_error("invalid data or unexpected end of file");
            }
            auto field = std::make_shared<std::vector<double>>();
            auto vertices = xsize * ysize * zsize;
            field->resize(vertices * N_);

            // Loop through all the field data
            for(size_t i = 0; i < vertices; ++i) {
                if(i % (vertices / 100) == 0) {
                    LOG_PROGRESS(INFO, "read_init") << "Reading field data: " << (100 * i / vertices) << "%";
                }

                if(file.eof()) {
                    throw std::runtime_error("unexpected end of file");
                }

                // Get index of field
                size_t xind, yind, zind;
                file >> xind >> yind >> zind;

                if(file.fail() || xind > xsize || yind > ysize || zind > zsize) {
                    throw std::runtime_error("invalid data");
                }
                xind--;
                yind--;
                zind--;

                // Loop through components of field
                for(size_t j = 0; j < N_; ++j) {
                    double input;
                    file >> input;

                    // Set the field at a position
                    (*field)[xind * ysize * zsize * N_ + yind * zsize * N_ + zind * N_ + j] = Units::get(input, units_);
                }
            }
            LOG_PROGRESS(INFO, "read_init") << "Reading field data: finished.";

            FieldData<T> field_data(
                header, std::array<size_t, 3>{{xsize, ysize, zsize}}, std::array<T, 3>{{xpixsz, ypixsz, thickness}}, field);

            // Store the parsed field data for further reference:
            field_map_[file_name] = field_data;
            return field_data;
        }

        size_t N_;
        std::string units_;
        std::map<std::string, FieldData<T>> field_map_;
    };

    template <typename T = double> class FieldWriter {
    public:
        FieldWriter(const FieldQuantity quantity, const std::string units = std::string()) : units_(std::move(units)) {
            // Store quantity: vector or scalar field:
            N_ = static_cast<std::underlying_type<FieldQuantity>::type>(quantity);
        };
        ~FieldWriter() = default;

        /**
         * @brief Write the field to a file
         */
        void write_file(const FieldData<T>& field_data, const std::string& file_name, const FileType& file_type) {
            auto dimensions = field_data.getDimensions();
            if(field_data.getData()->size() != N_ * dimensions[0] * dimensions[1] * dimensions[2]) {
                throw std::runtime_error("invalid field dimensions");
            }

            switch(file_type) {
            case FileType::INIT:
                write_init_file(field_data, file_name);
                break;
            case FileType::APF:
                write_apf_file(field_data, file_name);
                break;
            default:
                throw std::runtime_error("unknown file format");
            }
        }

    private:
        void write_apf_file(const FieldData<T>& field_data, const std::string& file_name) {
            if(!units_.empty()) {
                LOG(WARNING) << "Units will be ignored, APF file content is written in internal units.";
            }

            std::ofstream file(file_name, std::ios::binary);

            // Write the file with cereal:
            cereal::PortableBinaryOutputArchive archive(file);
            archive(field_data);
        }

        void write_init_file(const FieldData<T>& field_data, const std::string& file_name) {
            std::ofstream file(file_name);

            LOG(TRACE) << "Writing INIT file \"" << file_name << "\"";

            // Write INIT file header
            file << field_data.getHeader() << std::endl;  // Header line
            file << "##SEED## ##EVENTS##" << std::endl;   // Unused
            file << "##TURN## ##TILT## 1.0" << std::endl; // Unused
            file << "0.0 0.0 0.0" << std::endl;           // Magnetic field (unused)

            auto size = field_data.getSize();
            file << Units::convert(size[2], "um") << " " << Units::convert(size[0], "um") << " "
                 << Units::convert(size[1], "um") << " "; // Field size: (z, x, y)
            file << "0.0 0.0 0.0 0.0 ";                   // Unused

            auto dimensions = field_data.getDimensions();
            file << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << " "; // Field grid dimensions (x, y, z)
            file << "0.0" << std::endl;                                                   // Unused

            // Write the data block:
            auto data = field_data.getData();
            auto max_points = data->size() / N_;

            for(size_t xind = 0; xind < dimensions[0]; ++xind) {
                for(size_t yind = 0; yind < dimensions[1]; ++yind) {
                    for(size_t zind = 0; zind < dimensions[2]; ++zind) {
                        // Write field point index
                        file << xind + 1 << " " << yind + 1 << " " << zind + 1;

                        // Vector or scalar field:
                        for(size_t j = 0; j < N_; j++) {
                            file << " " << Units::convert(data->at(xind * dimensions[1] * dimensions[2] * N_ +
                                                                   yind * dimensions[2] * N_ + zind * N_ + j),
                                                          units_);
                        }

                        // End this line
                        file << std::endl;
                    }

                    auto curr_point = xind * dimensions[1] * dimensions[2] + yind * dimensions[2];
                    LOG_PROGRESS(INFO, "write_init") << "Writing field data: " << (100 * curr_point / max_points) << "%";
                }
            }
            LOG_PROGRESS(INFO, "write_init") << "Writing field data: finished.";
        }

        size_t N_;
        std::string units_;
    };
} // namespace allpix

#endif /* ALLPIX_FIELD_PARSER_H */
