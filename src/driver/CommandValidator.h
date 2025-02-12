#ifndef COMMAND_VALIDATOR_H
#define COMMAND_VALIDATOR_H

#include <iostream>
#include <regex>
#include <unordered_map>
#include <unordered_set>

class CommandValidator {
public:
    CommandValidator();
    bool validateEndpoint(const std::string& endpoint);
    bool validateOperation(const std::string& endpoint);
    bool setState(const std::string& newState);
    std::string getState() const; 
    bool setConfig(const std::string& param, const std::string& value);
    bool setConfigFromEndpoint(const std::string& endpoint);
    std::string getConfig(const std::string& param) const; 

private:
    std::string currentState;
    std::unordered_map<std::string, std::string> configValues;
    std::unordered_map<std::string, std::string> standaloneCommands;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> validCommands;
    std::unordered_map<std::string, std::unordered_set<std::string>> validOperations;

};

#endif // COMMAND_VALIDATOR_H
