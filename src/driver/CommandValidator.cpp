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
}

bool CommandValidator::validateCommand(const std::string& input) {
    // Check if it's a standalone command (e.g., "ping", "reset", "trigger")
    if (standaloneCommands.find(input) != standaloneCommands.end()) {
        return true;
    }

    // Regex to match formats: command=value OR command=parameter:value
    std::regex pattern(R"(^([\w]+)(?:=([\w]+)(?::([\w\.\+\-]+))?)?$)");
    std::smatch match;

    if (!std::regex_match(input, match, pattern)) {
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

    // Validate value using regex
    std::regex valuePattern(validCommands[command][param]);
    return std::regex_match(value, valuePattern);
}
