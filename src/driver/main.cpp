#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <thread>
#include <chrono>

#include "HardwareAPI.h"

#define PORT 6001

HardwareAPI api("http://hardware:5001/");

void send_response(int client_socket, const std::string &response)
{
    ssize_t bytes_sent = send(client_socket, response.c_str(), response.size(), 0);
}

void handle_client(int client_socket)
{
    char buffer[1024];

    while (true)
    {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0)
        {
            std::cout << "[Socket] << " << buffer << std::endl;
            std::string response = api.send(buffer);
            send_response(client_socket, response);
        }
        else
        {
            break;
        }
    }

    close(client_socket);
    std::cout << "[Socket] Connection closed." << std::endl;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    std::memset(&(address.sin_zero), '\0', 8);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 3);

    std::cout << "[Socket] Driver (C++ server) listening on port " << std::to_string(PORT) << "..." << std::endl;

    while (true)
    {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        std::cout << "[Socket] Client connected." << std::endl;

        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }

    close(server_fd);
    return 0;
}
