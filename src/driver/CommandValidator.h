#ifndef COMMAND_VALIDATOR_H
#define COMMAND_VALIDATOR_H

#include <iostream>
#include <regex>
#include <unordered_map>

class CommandValidator {
public:
    CommandValidator();
    bool validateCommand(const std::string& input);

private:
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> validCommands;
    std::unordered_map<std::string, std::string> standaloneCommands;
};

#endif // COMMAND_VALIDATOR_H
