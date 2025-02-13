#include "CommandValidator.h"

CommandValidator::CommandValidator() {
    // Default state is idle
    currentState = "idle";

    // Allowed commands per state
    validCommands["idle"] = {"ping", "trigger", "get_frame", "get_state", "set_state", "reset"};
    validCommands["config"] = {"ping", "get_state", "set_state", "get_config", "set_config", "reset"};
    validCommands["capturing"] = {"ping", "get_state", "reset"};

    // Allowed parameters for /set_config with regex validation rules
    validConfigParams["focus"] = R"(^([0-9]|[1-9][0-9]{1,2}|1[0-5][0-9]{2}|1600)$)";  // Integer: 0-1600
    validConfigParams["exposure"] = R"(^([0-9]{1,3}(\.[0-9])?|1000\.0)$)";  // Float: 0.0-1000.0
    validConfigParams["gain"] = R"(^[+-](12|1[01]|[0-9])$)";  // Signed Integer: -12 to +12
    validConfigParams["led_pattern"] = R"(^[a-h]$)";  // Enum: a-h
    validConfigParams["led_intensity"] = R"(^([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)";  // Integer: 0-255
    validConfigParams["photometric_mode"] = R"(^[0-1]$)";  // Enum: 0 or 1

    // Default configuration values
    configValues["photometric_mode"] = "0";  // Default to off
    configValues["led_pattern"] = "a";  // Default to a (ALL OFF)
}

bool CommandValidator::validateEndpoint(const std::string& endpoint) {
    // Check if it's a simple command allowed in the current state
    if (validCommands[currentState].count(endpoint)) {
        return true;
    }

    // Validate /set_state
    std::regex setStatePattern(R"(^set_state=(\w+)$)");
    std::smatch match;
    if (std::regex_match(endpoint, match, setStatePattern)) {
        std::string newState = match[1];
        return setState(newState);
    }

    // Validate /get_config
    std::regex getConfigPattern(R"(^get_config=(\w+)$)");
    if (std::regex_match(endpoint, match, getConfigPattern)) {
        return currentState == "config" && configValues.find(match[1]) != configValues.end();
    }

    // Validate /set_config
    std::regex setConfigPattern(R"(^set_config=(\w+):([\w\.\+\-]+)$)");
    if (std::regex_match(endpoint, match, setConfigPattern)) {
        std::string param = match[1];
        std::string value = match[2];
        return setConfig(param, value);
    }

    return false;  // Invalid command or not allowed in the current state
}

bool CommandValidator::setState(const std::string& newState) {
    // Prevent setting state to capturing
    if (newState == "capturing") {
        return false;
    }

    // Check if the newState is valid
    if (validCommands.find(newState) != validCommands.end()) {
        currentState = newState;
        return true;
    }

    return false;
}

bool CommandValidator::setConfig(const std::string& param, const std::string& value) {
    // Ensure we are in config state
    if (currentState != "config") {
        return false;
    }

    // Check if the parameter is valid
    if (validConfigParams.find(param) == validConfigParams.end()) {
        return false;
    }

    // Validate the value against the regex pattern
    std::regex valuePattern(validConfigParams[param]);
    if (!std::regex_match(value, valuePattern)) {
        return false;
    }

    // Prevent illegal operations
    if (param == "photometric_mode" && value == "1" && configValues["led_pattern"] != "a") {
        return false;  // `photometric_mode` can only be set to 1 if `led_pattern` is "a"
    }
    if (param == "led_pattern" && configValues["photometric_mode"] == "1") {
        return false;  // `led_pattern` cannot be changed if `photometric_mode` is "1"
    }

    // Set the value
    configValues[param] = value;
    return true;
}

bool CommandValidator::setConfigFromEndpoint(const std::string& endpoint) {
    // Regex to match formats: command=value OR command=parameter:value
    std::regex pattern(R"(^([\w]+)(?:=([\w]+)(?::([\w\.\+\-]+))?)?$)");
    std::smatch match;

    if (!std::regex_match(endpoint, match, pattern)) {
        return false;  // Invalid format
    }

    std::string command = match[1];  // Extract command
    std::string param = match[2];    // Extract parameter (optional)
    std::string value = match[3];    // Extract value (optional)

    return setConfig(param, value);
}

std::string CommandValidator::getConfig(const std::string& param) const {
    if (configValues.find(param) != configValues.end()) {
        return configValues.at(param);
    }
    return "";
}

std::string CommandValidator::getState() const {
    return currentState;
}