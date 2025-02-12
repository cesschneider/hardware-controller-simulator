#include "CommandValidator.h"

CommandValidator::CommandValidator() {
    // Standalone commands (No parameters)
    standaloneCommands["ping"] = "";
    standaloneCommands["reset"] = "";
    standaloneCommands["trigger"] = "";
    standaloneCommands["get_state"] = "";
    standaloneCommands["get_frame"] = "";

    // State commands
    validCommands["set_state"][""] = "^(idle|config)$";  // Only "idle" or "config"

    // Get commands (parameters with no additional values)
    validCommands["get_config"][""] = "^(focus|exposure|gain|led_pattern|led_intensity|photometric_mode)$";

    // Set commands with numeric validation
    validCommands["set_config"]["focus"] = R"(^([0-9]|[1-9][0-9]{1,2}|1[0-5][0-9]{2}|1600)$)";  // 0-1600
    validCommands["set_config"]["exposure"] = R"(^([0-9]{1,3}(\.[0-9])?|1000\.0)$)";  // 0.0 - 1000.0 (one decimal)
    validCommands["set_config"]["gain"] = R"(^([-+]?(12|1[01]|[0-9]))$)";  // -12 to +12
    validCommands["set_config"]["led_pattern"] = R"(^[a-h]$)";  // a-h
    validCommands["set_config"]["led_intensity"] = R"(^([0-9]|[1-9][0-9]|1[0-9]{2}|2[0-4][0-9]|25[0-5])$)";  // 0-255
    validCommands["set_config"]["photometric_mode"] = R"(^[0-1]$)";  // 0 or 1

    // Allowed commands in each state
    validOperations["idle"] = {"ping", "trigger", "get_frame", "get_state", "set_state", "reset"};
    validOperations["config"] = {"ping", "get_state", "set_state", "get_config", "set_config", "reset"};
    validOperations["capturing"] = {"ping", "get_state", "reset"};

    // Default configuration values
    configValues["photometric_mode"] = "0";  // Default to off
    configValues["led_pattern"] = "a";  // Default to a (ALL OFF)
}

bool CommandValidator::setState(const std::string& newState) {
    fprintf(stdout, "[CommandValidator] setConfig(): Setting state '%s'\n", newState.c_str());

    // Check if the newState is valid
    if (validCommands["set_state"].find(newState) != validCommands["set_state"].end()) {
        currentState = newState;
        return true;
    }

    fprintf(stderr, "[CommandValidator] setConfig(): Invalid state '%s'\n", newState.c_str());
    return false;
}

bool CommandValidator::setConfig(const std::string& param, const std::string& value) {
    fprintf(stdout, "[CommandValidator] setConfig(): Setting config '%s' with value '%s'\n", 
        param.c_str(), value.c_str());

    // Ensure we are in config state
    if (currentState != "config") {
        return false;
    }

    // Check if the parameter is valid
    if (validCommands["set_config"].find(param) == validCommands["set_config"].end()) {
        return false;
    }

    // Validate the value against the regex pattern
    std::regex valuePattern(validCommands["set_config"][param]);
    if (!std::regex_match(value, valuePattern)) {
        return false;
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

bool CommandValidator::validateEndpoint(const std::string& endpoint) {
    // Check if it's a standalone command (e.g., "ping", "reset", "trigger")
    if (standaloneCommands.find(endpoint) != standaloneCommands.end()) {
        return true;
    }

    // Regex to match formats: command=value OR command=parameter:value
    std::regex pattern(R"(^([\w]+)(?:=([\w]+)(?::([\w\.\+\-]+))?)?$)");
    std::smatch match;

    if (!std::regex_match(endpoint, match, pattern)) {
        return false;  // Invalid format
    }

    std::string command = match[1];  // Extract command
    std::string param = match[2];    // Extract parameter (optional)
    std::string value = match[3];    // Extract value (optional)

    // Check if the command exists
    if (validCommands.find(command) == validCommands.end()) {
        return false;
    }

    // If the command requires only a value (e.g., "set_state=idle")
    if (validCommands[command].size() == 1 && validCommands[command].count("")) {
        std::regex valuePattern(validCommands[command][""]);
        return std::regex_match(param, valuePattern);  // Param here is the actual value
    }

    // Check if the parameter is valid for the command
    if (validCommands[command].find(param) == validCommands[command].end()) {
        return false;
    }

    // Enforce interdependencies
    if (param == "photometric_mode" && value == "1" && configValues["led_pattern"] != "a") {
        return false;  // photometric_mode can only be set to 1 if led_pattern is "a"
    }
    if (param == "led_pattern" && configValues["photometric_mode"] == "1") {
        return false;  // led_pattern cannot be changed if photometric_mode is "1"
    }

    // Validate value using regex
    std::regex valuePattern(validCommands[command][param]);
    return std::regex_match(value, valuePattern);
}

bool CommandValidator::validateOperation(const std::string& endpoint) {

    // Enforce that trigger and get_frame can only be used in idle
    if ((endpoint == "trigger" || endpoint == "get_frame") && currentState != "idle") {
        return false;
    }

    return true;
}