#include "ServerHandler.hpp"

void ServerHandler::Server(std::mutex* valueLock)
{
    DataValues* data = DataValues::Get();

    std::map<float, std::string> valuesUpdateCache;
    
    nlohmann::json goGridLabelArray;

    for (const auto& row : UI::Get()->go_grid_labels) {
        nlohmann::json jsonRow = nlohmann::json::array();
        for (auto element : row) {
            jsonRow.push_back(element);
        }
        goGridLabelArray.push_back(jsonRow);
    }

    httplib::Server svr;

    auto ret = svr.set_mount_point("/assets", "./assets");

    svr.Get("/", [](const httplib::Request &req, httplib::Response &res) {
        res.set_file_content("./assets/index.html", "text/html");
    });

    svr.Get("/favicon.ico", [](const httplib::Request &req, httplib::Response &res) {
        res.set_file_content("./assets/icon.png", "image/png");
    });

    svr.Get("/data.json/:id", [&valueLock, &data, &valuesUpdateCache, goGridLabelArray](const httplib::Request &req, httplib::Response &res) {
        valueLock->lock();

        float lastTimeStamp = std::stof(req.path_params.at("id"));
        std::map<float, DataValues::DataValueSnapshot>* dataMap = data->getValueSnapshotMap();

        std::map<float, nlohmann::json> filteredMap;
        
        std::map<std::string, std::string> jsonMap;

        std::string valuesUpdateString;

        if(valuesUpdateCache.count(lastTimeStamp) == 0 || valuesUpdateCache[lastTimeStamp] == "[]") 
        {
            auto filterStart = dataMap->upper_bound(lastTimeStamp);
            for (auto current = filterStart; current != dataMap->end(); current++) {
                nlohmann::json j;
                current->second.to_json(j, current->second);
                filteredMap[current->first] = j;
            }
            nlohmann::json valuesUpdate = filteredMap;
            valuesUpdateString = valuesUpdate.dump();
            valuesUpdateCache[lastTimeStamp] = valuesUpdateString;
        }
        else
        {
            valuesUpdateString = valuesUpdateCache[lastTimeStamp];
        }

        nlohmann::json goGridValueArray;
        for (const auto& row : data->go_grid_values) {
            nlohmann::json jsonRow = nlohmann::json::array();
            for (int element : row) {
                jsonRow.push_back(element);
            }
            goGridValueArray.push_back(jsonRow);
        }

        nlohmann::json countdownStartTime = data->coundown_start_time;
        nlohmann::json countdownTime = data->fuse_delay;

        jsonMap["countdownTime"] = countdownTime.dump();
        jsonMap["countdownStartTime"] = countdownStartTime.dump();
        jsonMap["valuesUpdate"] = valuesUpdateString;
        jsonMap["goGridValues"] = goGridValueArray.dump();
        
        if(lastTimeStamp <= 1) 
        {
            jsonMap["goGridLabels"] = goGridLabelArray.dump();
        }

        valueLock->unlock();

        nlohmann::json finalJson = jsonMap;

        res.set_content(finalJson.dump(), "application/json");
    });

    svr.listen("0.0.0.0", 8080);
}