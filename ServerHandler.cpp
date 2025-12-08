#include "ServerHandler.hpp"

void ServerHandler::Server(std::mutex* valueLock)
{
    DataValues* data = DataValues::Get();

    httplib::Server svr;

    auto ret = svr.set_mount_point("/assets", "./assets");

    svr.Get("/", [](const httplib::Request &, httplib::Response &res) {
        res.set_file_content("./assets/index.html", "text/html");
    });

    svr.Get("/favicon.ico", [](const httplib::Request &, httplib::Response &res) {
        res.set_file_content("./assets/icon.png", "image/png");
    });

    svr.Get("/go-grid.txt", [&valueLock, &data](const httplib::Request &, httplib::Response &res) {
        std::string goGridText;

        valueLock->lock();

        for(int i = 0; i < 5; i++){
            for(int j = 0; j < 5; j++){
                goGridText += "(";
                goGridText += std::to_string(data->go_grid_values[i][j]);
                goGridText += ",";
                goGridText += UI::Get()->go_grid_labels[j][i];
                goGridText += "),";
            }
        } 

        if(data->coundown_start_time != NULL)
        {
            time_t current_time_t = time(NULL);
            int time_since_countdown = difftime(data->coundown_start_time, current_time_t);
            int countdown = data->fuse_delay - std::abs(time_since_countdown);

            goGridText += std::to_string(countdown);
        }
	    else
        {
            goGridText += "N/A";
        }
        
        valueLock->unlock();


        res.set_content(goGridText, "text/plain");
    });

    svr.listen("0.0.0.0", 8080);
}