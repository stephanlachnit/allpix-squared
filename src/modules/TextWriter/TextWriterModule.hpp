/**
 * @file
 * @brief Definition of ASCII text file writer module
 * @copyright Copyright (c) 2017-2020 CERN and the Allpix Squared authors.
 * This software is distributed under the terms of the MIT License, copied verbatim in the file "LICENSE.md".
 * In applying this license, CERN does not waive the privileges and immunities granted to it by virtue of its status as an
 * Intergovernmental Organization or submit itself to any jurisdiction.
 */

#include <fstream>
#include <map>
#include <string>

#include "core/config/Configuration.hpp"
#include "core/geometry/GeometryManager.hpp"
#include "core/messenger/Messenger.hpp"
#include "core/module/Module.hpp"

namespace allpix {
    /**
     * @ingroup Modules
     * @brief Module to write object data to simple ASCII text files
     *
     * Listens to all objects dispatched in the framework and stores an ASCII representation of every object to file.
     */
    class TextWriterModule : public Module {
    public:
        /**
         * @brief Constructor for this unique module
         * @param config Configuration object for this module as retrieved from the steering file
         * @param messenger Pointer to the messenger object to allow binding to messages on the bus
         * @param geo_mgr Pointer to the geometry manager, containing the detectors
         */
        TextWriterModule(Configuration& config, Messenger* messenger, GeometryManager* geo_mgr);
        /**
         * @brief Destructor deletes the internal objects used to build the ROOT Tree
         */
        ~TextWriterModule() override;

        /**
         * @brief Receive a single message containing objects of arbitrary type
         * @param message Message dispatched in the framework
         * @param name Name of the message
         */
        void receive(std::shared_ptr<BaseMessage> message, std::string name);

        /**
         * @brief Opens the file to write the objects to
         */
        void init() override;

        /**
         * @brief Writes the objects fetched to their specific tree, constructing trees on the fly for new objects.
         */
        void run(unsigned int) override;

        /**
         * @brief Add the main configuration and the detector setup to the data file and write it, also write statistics
         * information.
         */
        void finalize() override;

    private:
        // Object names to include or exclude from writing
        std::set<std::string> include_;
        std::set<std::string> exclude_;

        // Output data file to write
        std::string output_file_name_{};
        std::unique_ptr<std::ofstream> output_file_;

        // List of messages to keep so they can be stored in the tree
        std::vector<std::shared_ptr<BaseMessage>> keep_messages_;
        // List of objects of a particular type, bound to a specific detector and having a particular name
        std::map<std::tuple<std::type_index, std::string, std::string>, std::vector<Object*>*> write_list_;

        // Statistical information about number of objects
        unsigned long write_cnt_{};
        unsigned long msg_cnt_{};
    };
} // namespace allpix
