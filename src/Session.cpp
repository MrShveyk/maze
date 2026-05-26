/** @file Session.cpp
 *  Реализация игровой сессии.
 */

#include "Session.hpp"

#include <cstring>
#include <iostream>
#include <stdexcept>

#include <sys/socket.h>
#include <unistd.h>

#include "Protocol.hpp"

namespace p2p_game {

Session::Session(int client_socket, const SessionConfig& cfg)
    : socket_(client_socket),
      cfg_(cfg),
      moves_left_(cfg.max_moves),
      perf_mode_(false),
      maze_(cfg.maze_size) {
    buffer_[0] = '\0';
}

Session::~Session() {
    if (socket_ >= 0) {
        close(socket_);
    }
}

std::string Session::recvLine() {
    std::string msg;
    while (true) {
        const ssize_t n = recv(socket_, buffer_, sizeof(buffer_) - 1, 0);
        if (n <= 0) {
            throw std::runtime_error("Session: клиент отключился");
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

void Session::sendLine(const std::string& msg) {
    const std::string framed = msg + "\n";
    const char* data = framed.data();
    size_t left = framed.size();
    while (left > 0) {
        const ssize_t n = send(socket_, data, left, 0);
        if (n <= 0) {
            throw std::runtime_error("Session: ошибка отправки");
        }
        data += n;
        left -= static_cast<size_t>(n);
    }
}

void Session::handleInit(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        sendLine(protocol::makeErr("ожидается HELLO/PERF <имя>"));
        throw std::runtime_error("Session: некорректный init");
    }
    const std::string& cmd = tokens[0];
    player_name_ = tokens[1];

    if (cmd == protocol::REQ_PERF) {
        perf_mode_ = true;
        moves_left_ = -1;
    } else if (cmd == protocol::REQ_HELLO) {
        perf_mode_ = false;
        moves_left_ = cfg_.max_moves;
    } else {
        sendLine(protocol::makeErr("ожидается HELLO или PERF"));
        throw std::runtime_error("Session: неизвестная init-команда");
    }
}

bool Session::cmdToDelta(const std::string& cmd, int& dx, int& dy) {
    if (cmd == protocol::REQ_FORWARD) { dx = 0;  dy = 1;  return true; }
    if (cmd == protocol::REQ_BACK)    { dx = 0;  dy = -1; return true; }
    if (cmd == protocol::REQ_LEFT)    { dx = -1; dy = 0;  return true; }
    if (cmd == protocol::REQ_RIGHT)   { dx = 1;  dy = 0;  return true; }
    return false;
}

std::string Session::handleCommand(const std::string& cmd) {
    if (cmd == protocol::REQ_GIVEUP) {
        return protocol::RES_LOSE;
    }

    int dx = 0;
    int dy = 0;
    if (!cmdToDelta(cmd, dx, dy)) {
        return protocol::makeErr("неизвестная команда");
    }

    const bool moved = maze_.move(dx, dy);

    if (!perf_mode_) {
        --moves_left_;
    }

    if (moved) {
        if (maze_.isWin()) {
            return protocol::RES_WIN;
        }
        if (!perf_mode_ && moves_left_ <= 0) {
            return protocol::RES_LOSE;
        }
        return protocol::makeOk(maze_.playerX(), maze_.playerY(), moves_left_);
    }

    if (!perf_mode_ && moves_left_ <= 0) {
        return protocol::RES_LOSE;
    }
    return protocol::makeWall(moves_left_);
}

void Session::run() {
    const std::string init = recvLine();
    const auto tokens = protocol::tokenize(init);
    handleInit(tokens);

    maze_.generate(cfg_.wall_count);
    sendLine(protocol::makeSize(maze_.size()));

    std::cout << "[session " << getpid() << "] игрок '" << player_name_
              << (perf_mode_ ? "' (тест.режим)" : "'")
              << ", поле " << maze_.size() << "x" << maze_.size()
              << std::endl;

    while (true) {
        const std::string line = recvLine();
        const auto cmd_tokens = protocol::tokenize(line);
        if (cmd_tokens.empty()) {
            continue;
        }
        const std::string reply = handleCommand(cmd_tokens[0]);
        sendLine(reply);

        if (reply == protocol::RES_WIN || reply == protocol::RES_LOSE ||
            reply.rfind(protocol::RES_ERR, 0) == 0) {
            break;
        }
    }
}

} // namespace p2p_game
