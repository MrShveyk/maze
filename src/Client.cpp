/** @file Client.cpp
 *  Реализация консольного клиента игры «Лабиринт».
 */

#include "Client.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>
#include <utility>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "Protocol.hpp"

namespace p2p_game {

Client::Client(std::string server_ip,
               unsigned short port,
               std::string player_name,
               bool perf_mode)
    : server_ip_(std::move(server_ip)),
      port_(port),
      player_name_(std::move(player_name)),
      perf_mode_(perf_mode),
      socket_(-1),
      moves_left_(0),
      game_over_(false),
      tty_saved_(false) {
    buffer_[0] = '\0';
}

Client::~Client() {
    if (socket_ >= 0) {
        close(socket_);
    }
    restoreTerminal();
}

void Client::connectToServer() {
    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        throw std::runtime_error("socket: не удалось создать сокет");
    }

    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo* result = nullptr;
    const int gai = getaddrinfo(server_ip_.c_str(), nullptr, &hints, &result);
    if (gai != 0 || result == nullptr) {
        throw std::runtime_error(std::string("getaddrinfo: ") + gai_strerror(gai));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    std::memcpy(&addr.sin_addr,
                &reinterpret_cast<sockaddr_in*>(result->ai_addr)->sin_addr,
                sizeof(addr.sin_addr));
    addr.sin_port = htons(port_);
    freeaddrinfo(result);

    if (::connect(socket_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        throw std::runtime_error(std::string("connect: ") + std::strerror(errno));
    }
}

void Client::sendInit() {
    sendLine(perf_mode_ ? protocol::makePerf(player_name_)
                        : protocol::makeHello(player_name_));
}

void Client::sendLine(const std::string& msg) {
    const std::string framed = msg + "\n";
    const char* data = framed.data();
    size_t left = framed.size();
    while (left > 0) {
        const ssize_t n = send(socket_, data, left, 0);
        if (n <= 0) {
            throw std::runtime_error("Client: ошибка отправки");
        }
        data += n;
        left -= static_cast<size_t>(n);
    }
}

std::string Client::recvLine() {
    std::string msg;
    while (true) {
        const ssize_t n = recv(socket_, buffer_, sizeof(buffer_) - 1, 0);
        if (n <= 0) {
            throw std::runtime_error("Client: соединение закрыто сервером");
        }
        buffer_[n] = '\0';
        msg += buffer_;
        const size_t pos = msg.find('\n');
        if (pos != std::string::npos) {
            msg.resize(pos);
            if (!msg.empty() && msg.back() == '\r') {
                msg.pop_back();
            }
            return msg;
        }
    }
}

void Client::setupTerminal() {
    if (tcgetattr(STDIN_FILENO, &saved_tty_) != 0) {
        return;
    }
    tty_saved_ = true;
    termios tty = saved_tty_;
    tty.c_lflag &= ~(ICANON | ECHO);
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    std::cout << "\033[?25l" << std::flush;
}

void Client::restoreTerminal() {
    if (tty_saved_) {
        tcsetattr(STDIN_FILENO, TCSANOW, &saved_tty_);
        std::cout << "\033[?25h" << std::flush;
        tty_saved_ = false;
    }
}

char Client::readKey() {
    char c = '\0';
    if (read(STDIN_FILENO, &c, 1) > 0) {
        return c;
    }
    return '\0';
}

void Client::recordWallFromCollision(const std::string& cmd) {
    const int px = maze_.playerX();
    const int py = maze_.playerY();
    int tx = px;
    int ty = py;
    if (cmd == protocol::REQ_FORWARD)      ty = py + 1;
    else if (cmd == protocol::REQ_BACK)    ty = py - 1;
    else if (cmd == protocol::REQ_LEFT)    tx = px - 1;
    else if (cmd == protocol::REQ_RIGHT)   tx = px + 1;
    else return;
    maze_.addWall(px, py, tx, ty);
}

void Client::processResponse(const std::string& response, const std::string& cmd) {
    const auto tokens = protocol::tokenize(response);
    if (tokens.empty()) {
        return;
    }
    const std::string& tag = tokens[0];

    if (tag == protocol::RES_OK) {
        if (tokens.size() >= 3) {
            maze_.setPlayerPosition(std::stoi(tokens[1]), std::stoi(tokens[2]));
        }
        if (tokens.size() >= 4) {
            moves_left_ = std::stoi(tokens[3]);
        }
    } else if (tag == protocol::RES_WALL) {
        recordWallFromCollision(cmd);
        if (tokens.size() >= 2) {
            moves_left_ = std::stoi(tokens[1]);
        }
    } else if (tag == protocol::RES_WIN) {
        game_over_ = true;
        final_msg_ = "ПОБЕДА!";
    } else if (tag == protocol::RES_LOSE) {
        game_over_ = true;
        final_msg_ = "ПОРАЖЕНИЕ";
    } else if (tag == protocol::RES_ERR) {
        game_over_ = true;
        final_msg_ = "ОШИБКА: " + response;
    }
}

void Client::renderUI() {
    std::cout << "\033[H\033[0J";
    std::cout << "============= ЛАБИРИНТ =============\n";
    std::cout << " Игрок: " << player_name_;
    if (perf_mode_) {
        std::cout << "  [режим тестирования]";
    }
    std::cout << "\n";
    if (!perf_mode_) {
        std::cout << " Осталось ходов: " << moves_left_ << "\n";
    }
    std::cout << "\n";

    maze_.print(std::cout);
    std::cout << "\n";
    std::cout << " P - игрок,  G - цель\n";
    std::cout << " | / --- : стена,   пробел : проход\n\n";
    std::cout << " [w] вперёд  [s] назад  [a] влево  [d] вправо  [q] сдаться\n\n";

    std::cout << " --- последние ходы ---\n";
    const int start = static_cast<int>(history_.size()) > 6
                          ? static_cast<int>(history_.size()) - 6
                          : 0;
    for (size_t i = static_cast<size_t>(start); i < history_.size(); ++i) {
        std::cout << "  [" << (i + 1) << "] "
                  << history_[i].command << " -> " << history_[i].response
                  << "\n";
    }

    if (game_over_) {
        std::cout << "\n  >>> " << final_msg_ << " <<<\n";
    }
    std::cout.flush();
}

void Client::run() {
    setupTerminal();
    connectToServer();
    sendInit();

    std::cout << "\033[2J\033[H";

    const std::string size_msg = recvLine();
    const auto size_tokens = protocol::tokenize(size_msg);
    if (size_tokens.size() >= 2 && size_tokens[0] == protocol::RES_SIZE) {
        maze_.setSize(static_cast<unsigned>(std::stoi(size_tokens[1])));
    } else if (!size_tokens.empty() && size_tokens[0] == protocol::RES_ERR) {
        throw std::runtime_error("сервер вернул ошибку: " + size_msg);
    } else {
        throw std::runtime_error("ожидался ответ SIZE, получено: " + size_msg);
    }

    renderUI();

    while (!game_over_) {
        char key = '\0';
        while (key == '\0') {
            key = readKey();
            if (key == '\0') {
                usleep(30000);
            }
        }

        std::string cmd;
        switch (key) {
            case 'w': cmd = protocol::REQ_FORWARD; break;
            case 's': cmd = protocol::REQ_BACK;    break;
            case 'a': cmd = protocol::REQ_LEFT;    break;
            case 'd': cmd = protocol::REQ_RIGHT;   break;
            case 'q': cmd = protocol::REQ_GIVEUP;  break;
            default: continue;
        }

        sendLine(cmd);
        const std::string response = recvLine();
        history_.push_back({cmd, response});
        processResponse(response, cmd);
        renderUI();
    }

    restoreTerminal();
    std::cout << "\n";
}

} // namespace p2p_game
