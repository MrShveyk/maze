/** @file Server.cpp
 *  Реализация многопроцессного TCP-сервера.
 */

#include "Server.hpp"

#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <stdexcept>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

namespace p2p_game {

Server::Server(unsigned short port, const SessionConfig& cfg)
    : port_(port), cfg_(cfg), listen_socket_(-1) {}

Server::~Server() {
    if (listen_socket_ >= 0) {
        close(listen_socket_);
    }
}

void Server::onSigChld(int) {
    // Неблокирующая «сборка мусора»: пока есть завершившиеся потомки.
    int saved_errno = errno;
    while (waitpid(-1, nullptr, WNOHANG) > 0) {
    }
    errno = saved_errno;
}

void Server::setupSocket() {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        throw std::runtime_error(std::string("socket: ") + std::strerror(errno));
    }

    const int yes = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("bind: ") + std::strerror(errno));
    }
    if (listen(listen_socket_, SOMAXCONN) < 0) {
        throw std::runtime_error(std::string("listen: ") + std::strerror(errno));
    }
}

void Server::run() {
    struct sigaction sa{};
    sa.sa_handler = &Server::onSigChld;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa, nullptr) == -1) {
        throw std::runtime_error("sigaction(SIGCHLD)");
    }

    // SIGPIPE при разорванном соединении не должен убивать процесс.
    std::signal(SIGPIPE, SIG_IGN);

    setupSocket();

    std::cout << "[server] слушаем порт " << port_
              << ", поле " << cfg_.maze_size << "x" << cfg_.maze_size
              << ", стен " << cfg_.wall_count
              << ", лимит ходов " << cfg_.max_moves
              << ", seed=";
    if (cfg_.seed) {
        std::cout << *cfg_.seed << " (воспроизводимо)";
    } else {
        std::cout << "random";
    }
    std::cout << std::endl;

    while (true) {
        sockaddr_in client_addr{};
        socklen_t addr_len = sizeof(client_addr);

        const int client = accept(listen_socket_,
                                   reinterpret_cast<sockaddr*>(&client_addr),
                                   &addr_len);
        if (client < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "[server] accept: " << std::strerror(errno) << std::endl;
            continue;
        }

        const char* ip = inet_ntoa(client_addr.sin_addr);
        std::cout << "[server] подключение " << (ip ? ip : "?") << ":"
                  << ntohs(client_addr.sin_port) << std::endl;

        const pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "[server] fork: " << std::strerror(errno) << std::endl;
            close(client);
            continue;
        }
        if (pid == 0) {
            // Дочерний процесс — обслуживает клиента.
            close(listen_socket_);
            try {
                Session session(client, cfg_);
                session.run();
            } catch (const std::exception& e) {
                std::cerr << "[session " << getpid() << "] " << e.what() << std::endl;
            }
            _exit(0);
        }

        // Родительский процесс — закрывает клиентский сокет.
        close(client);
    }
}

} // namespace p2p_game
