//
// Created by 19396 on 25-5-15.
//

#include <httplib.h>

int main()
{
    httplib::Server server;
    server.Get("/", [](const httplib::Request &req, httplib::Response &resp) {
        std::cout << req.method << "\n";
        std::cout << req.path << "\n";
        for (auto& it: req.headers)
            std::cout << it.first << " " << it.second << "\n";
        std::string body = "<html><body><h1>Hello Bite</h1></body></html>";
        resp.set_content(body, "text/html");
        resp.status = 200;
    });

    server.listen("0.0.0.0", 8888);
    return 0;
}