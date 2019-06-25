// Copyright (c) 2017-2019 The Khronos Group Inc.
// Copyright (c) 2017-2019 Valve Corporation
// Copyright (c) 2017-2019 LunarG, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: Mark Young <marky@lunarg.com>
//

#include <sstream>
#include <iostream>

#include "loader_logger_recorders.hpp"

// Anonymous namespace to keep these types private
namespace {
// Standard Error logger, always on for now
class StdErrLoaderLogRecorder : public LoaderLogRecorder {
   public:
    StdErrLoaderLogRecorder(void* user_data);
    ~StdErrLoaderLogRecorder() {}

    bool LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity, XrLoaderLogMessageTypeFlags message_type,
                    const XrLoaderLogMessengerCallbackData* callback_data) override;
};

// Standard Output logger used with XR_LOADER_DEBUG
class StdOutLoaderLogRecorder : public LoaderLogRecorder {
   public:
    StdOutLoaderLogRecorder(void* user_data, XrLoaderLogMessageSeverityFlags flags);
    ~StdOutLoaderLogRecorder() {}

    bool LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity, XrLoaderLogMessageTypeFlags message_type,
                    const XrLoaderLogMessengerCallbackData* callback_data) override;
};

// Debug Utils logger used with XR_EXT_debug_utils
class DebugUtilsLogRecorder : public LoaderLogRecorder {
   public:
    DebugUtilsLogRecorder(const XrDebugUtilsMessengerCreateInfoEXT* create_info, XrDebugUtilsMessengerEXT debug_messenger);
    ~DebugUtilsLogRecorder() {}

    bool LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity, XrLoaderLogMessageTypeFlags message_type,
                    const XrLoaderLogMessengerCallbackData* callback_data) override;

    // Extension-specific logging functions
    bool LogDebugUtilsMessage(XrDebugUtilsMessageSeverityFlagsEXT message_severity, XrDebugUtilsMessageTypeFlagsEXT message_type,
                              const XrDebugUtilsMessengerCallbackDataEXT* callback_data) override;

   private:
    PFN_xrDebugUtilsMessengerCallbackEXT _user_callback;
};

// Standard Error logger, always on for now
StdErrLoaderLogRecorder::StdErrLoaderLogRecorder(void* user_data)
    : LoaderLogRecorder(XR_LOADER_LOG_STDERR, user_data, XR_LOADER_LOG_MESSAGE_SEVERITY_ERROR_BIT, 0xFFFFFFFFUL) {
    // Automatically start
    Start();
}

bool StdErrLoaderLogRecorder::LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity,
                                         XrLoaderLogMessageTypeFlags message_type,
                                         const XrLoaderLogMessengerCallbackData* callback_data) {
    if (_active && XR_LOADER_LOG_MESSAGE_SEVERITY_ERROR_BIT <= message_severity) {
        std::cerr << "Error [";
        switch (message_type) {
            case XR_LOADER_LOG_MESSAGE_TYPE_GENERAL_BIT:
                std::cerr << "GENERAL";
                break;
            case XR_LOADER_LOG_MESSAGE_TYPE_SPECIFICATION_BIT:
                std::cerr << "SPEC";
                break;
            case XR_LOADER_LOG_MESSAGE_TYPE_PERFORMANCE_BIT:
                std::cerr << "PERF";
                break;
            default:
                std::cerr << "UNKNOWN";
                break;
        }
        std::cerr << " | " << callback_data->command_name << " | " << callback_data->message_id << "] : " << callback_data->message
                  << std::endl;

        for (uint32_t obj = 0; obj < callback_data->object_count; ++obj) {
            std::cerr << "    Object[" << obj << "] = " << callback_data->objects[obj].ToString();
            std::cerr << std::endl;
        }
        for (uint32_t label = 0; label < callback_data->session_labels_count; ++label) {
            std::cerr << "    SessionLabel[" << std::to_string(label) << "] = " << callback_data->session_labels[label].labelName;
            std::cerr << std::endl;
        }
    }

    // Return of "true" means that we should exit the application after the logged message.  We
    // don't want to do that for our internal logging.  Only let a user return true.
    return false;
}

// Standard Output logger used with XR_LOADER_DEBUG
StdOutLoaderLogRecorder::StdOutLoaderLogRecorder(void* user_data, XrLoaderLogMessageSeverityFlags flags)
    : LoaderLogRecorder(XR_LOADER_LOG_STDOUT, user_data, flags, 0xFFFFFFFFUL) {
    // Automatically start
    Start();
}

bool StdOutLoaderLogRecorder::LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity,
                                         XrLoaderLogMessageTypeFlags message_type,
                                         const XrLoaderLogMessengerCallbackData* callback_data) {
    if (_active && 0 != (_message_severities & message_severity) && 0 != (_message_types & message_type)) {
        if (XR_LOADER_LOG_MESSAGE_SEVERITY_INFO_BIT > message_severity) {
            std::cout << "Verbose [";
        } else if (XR_LOADER_LOG_MESSAGE_SEVERITY_WARNING_BIT > message_severity) {
            std::cout << "Info [";
        } else if (XR_LOADER_LOG_MESSAGE_SEVERITY_ERROR_BIT > message_severity) {
            std::cout << "Warning [";
        } else {
            std::cout << "Error [";
        }
        switch (message_type) {
            case XR_LOADER_LOG_MESSAGE_TYPE_GENERAL_BIT:
                std::cout << "GENERAL";
                break;
            case XR_LOADER_LOG_MESSAGE_TYPE_SPECIFICATION_BIT:
                std::cout << "SPEC";
                break;
            case XR_LOADER_LOG_MESSAGE_TYPE_PERFORMANCE_BIT:
                std::cout << "PERF";
                break;
            default:
                std::cout << "UNKNOWN";
                break;
        }
        std::cout << " | " << callback_data->command_name << " | " << callback_data->message_id << "] : " << callback_data->message
                  << std::endl;

        for (uint32_t obj = 0; obj < callback_data->object_count; ++obj) {
            std::cout << "    Object[" << obj << "] = " << callback_data->objects[obj].ToString();
            std::cout << std::endl;
        }
        for (uint32_t label = 0; label < callback_data->session_labels_count; ++label) {
            std::cout << "    SessionLabel[" << std::to_string(label) << "] = " << callback_data->session_labels[label].labelName;
            std::cout << std::endl;
        }
    }

    // Return of "true" means that we should exit the application after the logged message.  We
    // don't want to do that for our internal logging.  Only let a user return true.
    return false;
}

// A logger associated with the XR_EXT_debug_utils extension

DebugUtilsLogRecorder::DebugUtilsLogRecorder(const XrDebugUtilsMessengerCreateInfoEXT* create_info,
                                             XrDebugUtilsMessengerEXT debug_messenger)
    : LoaderLogRecorder(XR_LOADER_LOG_DEBUG_UTILS, static_cast<void*>(create_info->userData),
                        DebugUtilsSeveritiesToLoaderLogMessageSeverities(create_info->messageSeverities),
                        DebugUtilsMessageTypesToLoaderLogMessageTypes(create_info->messageTypes)),
      _user_callback(create_info->userCallback) {
    // Use the debug messenger value to uniquely identify this logger with that messenger
    _unique_id = reinterpret_cast<uint64_t&>(debug_messenger);
    Start();
}

// Extension-specific logging functions
bool DebugUtilsLogRecorder::LogMessage(XrLoaderLogMessageSeverityFlagBits message_severity,
                                       XrLoaderLogMessageTypeFlags message_type,
                                       const XrLoaderLogMessengerCallbackData* callback_data) {
    bool should_exit = false;
    if (_active && 0 != (_message_severities & message_severity) && 0 != (_message_types & message_type)) {
        XrDebugUtilsMessageSeverityFlagsEXT utils_severity = DebugUtilsSeveritiesToLoaderLogMessageSeverities(message_severity);
        XrDebugUtilsMessageTypeFlagsEXT utils_type = LoaderLogMessageTypesToDebugUtilsMessageTypes(message_type);

        // Convert the loader log message into the debug utils log message information
        XrDebugUtilsMessengerCallbackDataEXT utils_callback_data = {};
        utils_callback_data.type = XR_TYPE_DEBUG_UTILS_MESSENGER_CALLBACK_DATA_EXT;
        utils_callback_data.messageId = callback_data->message_id;
        utils_callback_data.functionName = callback_data->command_name;
        utils_callback_data.message = callback_data->message;
        std::vector<XrDebugUtilsObjectNameInfoEXT> utils_objects;
        utils_objects.resize(callback_data->object_count);
        for (uint8_t object = 0; object < callback_data->object_count; ++object) {
            utils_objects[object].type = XR_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            utils_objects[object].next = nullptr;
            utils_objects[object].objectHandle = callback_data->objects[object].handle;
            utils_objects[object].objectType = callback_data->objects[object].type;
            utils_objects[object].objectName = callback_data->objects[object].name.c_str();
        }
        utils_callback_data.objectCount = callback_data->object_count;
        utils_callback_data.objects = utils_objects.data();
        utils_callback_data.sessionLabelCount = callback_data->session_labels_count;
        utils_callback_data.sessionLabels = callback_data->session_labels;

        // Call the user callback with the appropriate info
        // Return of "true" means that we should exit the application after the logged message.
        should_exit = (_user_callback(utils_severity, utils_type, &utils_callback_data, _user_data) == XR_TRUE);
    }

    return should_exit;
}

bool DebugUtilsLogRecorder::LogDebugUtilsMessage(XrDebugUtilsMessageSeverityFlagsEXT message_severity,
                                                 XrDebugUtilsMessageTypeFlagsEXT message_type,
                                                 const XrDebugUtilsMessengerCallbackDataEXT* callback_data) {
    // Call the user callback with the appropriate info
    // Return of "true" means that we should exit the application after the logged message.
    return (_user_callback(message_severity, message_type, callback_data, _user_data) == XR_TRUE);
}

}  // namespace

std::unique_ptr<LoaderLogRecorder> MakeStdOutLoaderLogRecorder(void* user_data, XrLoaderLogMessageSeverityFlags flags) {
    std::unique_ptr<LoaderLogRecorder> recorder(new StdOutLoaderLogRecorder(user_data, flags));
    return recorder;
}
std::unique_ptr<LoaderLogRecorder> MakeStdErrLoaderLogRecorder(void* user_data) {
    std::unique_ptr<LoaderLogRecorder> recorder(new StdErrLoaderLogRecorder(user_data));
    return recorder;
}
std::unique_ptr<LoaderLogRecorder> MakeDebugUtilsLoaderLogRecorder(const XrDebugUtilsMessengerCreateInfoEXT* create_info,
                                                                   XrDebugUtilsMessengerEXT debug_messenger) {
    std::unique_ptr<LoaderLogRecorder> recorder(new DebugUtilsLogRecorder(create_info, debug_messenger));
    return recorder;
}