#ifndef COMMAND_VALIDATOR_H
#define COMMAND_VALIDATOR_H

#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <regex>

class CommandValidator {
public:
    CommandValidator();  

    bool validateEndpoint(const std::string& endpoint);  
    bool setState(const std::string& newState);  
    std::string getState() const;  
    bool setConfig(const std::string& param, const std::string& value);  
    std::string getConfig(const std::string& param) const;  
    bool setConfigFromEndpoint(const std::string& endpoint);

private:
    std::string currentState;
    std::unordered_map<std::string, std::unordered_set<std::string>> validCommands;
    std::unordered_map<std::string, std::string> configValues;  
    std::unordered_map<std::string, std::string> validConfigParams;  
};

#endif // COMMAND_VALIDATOR_H
