#ifndef SERVER_H
#define SERVER_H

class Server {
public:
    explicit Server(int port);
    ~Server();

    void start();

private:
    int port_;
    int server_fd_;
    int epoll_fd_;

    void setupSocket();
    void setupEpoll();
    void eventLoop();
    void handleNewConnection();
    void handleClient(int client_fd);
};

#endif
