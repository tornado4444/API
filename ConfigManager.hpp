#pragma once
#include <string>
#include <fstream>
#include <json.h>

using json = nlohmann::json;

class ConfigManager {
private:
    json config;
    static ConfigManager* instance;

    ConfigManager() {
        std::ifstream file("appsettings.json");
        if (file.is_open()) {
            file >> config;
            file.close();
        }
        else {
            throw std::runtime_error("Cannot open appsettings.json");
        }
    }

public:
    static ConfigManager* getInstance() {
        if (instance == nullptr) {
            instance = new ConfigManager();
        }
        return instance;
    }

    std::string getConnectionString(const std::string& name = "ConnectDBConnection") {
        return config["ConnectionStrings"][name].get<std::string>();
    }
};
