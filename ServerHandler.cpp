#include "ServerHandler.hpp"

void ServerHandler::Server(std::mutex* valueLock)
{
    DataValues* data = DataValues::Get();

    std::map<float, std::string> valuesUpdateCache;
    
    nlohmann::json goGridLabelArray;

    valueLock->lock();
    for (const auto& row : UI::Get()->go_grid_labels) {
        nlohmann::json jsonRow = nlohmann::json::array();
        for (auto element : row) {
            jsonRow.push_back(element);
        }
        goGridLabelArray.push_back(jsonRow);
    }
    valueLock->unlock();

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

        std::map<float, DataValues::DataValueSnapshot> dataMap = *(data->getValueSnapshotMap());
        float goGridValues[5][5];
        memcpy(&goGridValues, data->go_grid_values, sizeof(goGridValues));
        
        nlohmann::json countdownStartTime = data->coundown_start_time;
        nlohmann::json countdownTime = data->fuse_delay;

        valueLock->unlock();

        float lastTimeStamp = std::stof(req.path_params.at("id"));


        std::string valuesUpdateString;

        if(valuesUpdateCache.count(lastTimeStamp) == 0 || valuesUpdateCache[lastTimeStamp] == "[]") 
        {
            std::map<float, nlohmann::json> filteredMap;
            auto filterStart = dataMap.upper_bound(lastTimeStamp);
            for (auto current = filterStart; current != dataMap.end(); current++) {
                nlohmann::json j;
                current->second.to_json(j, current->second);
                filteredMap[current->first] = j;
            }
            nlohmann::json valuesUpdate = filteredMap;
            valuesUpdateString = valuesUpdate.dump();
            valuesUpdateCache.emplace(lastTimeStamp, valuesUpdateString);
        }
        else
        {
            valuesUpdateString = valuesUpdateCache[lastTimeStamp];
        }

        nlohmann::json goGridValueArray;
        for (const auto& row : goGridValues) {
            nlohmann::json jsonRow = nlohmann::json::array();
            for (int element : row) {
                jsonRow.push_back(element);
            }
            goGridValueArray.push_back(jsonRow);
        }
        
        nlohmann::json finalJson;

        finalJson["countdownTime"] = countdownTime.dump();
        finalJson["countdownStartTime"] = countdownStartTime.dump();
        finalJson["valuesUpdate"] = valuesUpdateString;
        finalJson["goGridValues"] = goGridValueArray.dump();
        
        if(lastTimeStamp <= 1) 
        {
            finalJson["goGridLabels"] = goGridLabelArray.dump();
        }


        res.set_content(finalJson.dump(), "application/json");
    });

    svr.listen("0.0.0.0", 8080);
}