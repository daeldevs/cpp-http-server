#include "server.h"
#include <iostream>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <fcntl.h>


//constructor
Server::Server(int port)
    : port_(port), server_fd_(-1), epoll_fd_(-1) {}


//destructor
Server::~Server() {
    if (server_fd_ != -1) close(server_fd_);
    if (epoll_fd_ != -1) close(epoll_fd_);
}

void Server::start() {
    setupSocket();
    setupEpoll();
    eventLoop();
}

void Server::setupSocket() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ == -1) {
	perror("socket");
	exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    bind(server_fd_, (sockaddr*)&address, sizeof(address));
    listen(server_fd_, 10);

    fcntl(server_fd_, F_SETFL, O_NONBLOCK);

    std::cout << "Server listening on port " << port_ << "\n";
}

void Server::setupEpoll() {
    epoll_fd_ = epoll_create1(0);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = server_fd_;

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev);
}

void Server::eventLoop() {
    const int MAX_EVENTS = 10;
    epoll_event events[MAX_EVENTS];

    while (true) {
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

        for (int i = 0; i < n; i++) {

            if (events[i].data.fd == server_fd_) {
                handleNewConnection();
            } else {
                handleClient(events[i].data.fd);
            }
        }
    }
}

void Server::handleNewConnection() {
    int client_fd = accept(server_fd_, nullptr, nullptr);
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = client_fd;

    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev);
}

void Server::handleClient(int client_fd) {
    char buffer[4096];
    int bytes = read(client_fd, buffer, sizeof(buffer));

    if (bytes <= 0) {
        close(client_fd);
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
        return;
    }

    const char* response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 12\r\n"
        "\r\n"
        "Hello World!";

    write(client_fd, response, strlen(response));

    close(client_fd);
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
}


