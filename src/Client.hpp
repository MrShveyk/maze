/** @file Client.hpp
 *  Консольный TCP-клиент игры «Лабиринт».
 *
 *  Клиент подключается к серверу, выполняет handshake (HELLO/PERF),
 *  принимает размер поля и в цикле отправляет команды перемещения,
 *  обновляя локальную модель лабиринта по ответам сервера. Управление
 *  с клавиатуры — в неканоническом режиме терминала: w/a/s/d/q.
 */

#ifndef P2P_GAME_CLIENT_HPP
#define P2P_GAME_CLIENT_HPP

#include <string>
#include <vector>

#include <termios.h>

#include "Maze.hpp"

namespace p2p_game {

/** Запись истории: команда игрока и ответ сервера. */
struct HistoryEntry {
    std::string command;
    std::string response;
};

/** Консольный клиент игры. */
class Client {
public:
    Client(std::string server_ip,
           unsigned short port,
           std::string player_name,
           bool perf_mode);

    ~Client();

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /** Запуск клиента — установление соединения и игровой цикл. */
    void run();

private:
    void connectToServer();
    void sendInit();
    void sendLine(const std::string& msg);
    std::string recvLine();

    void renderUI();
    void processResponse(const std::string& response, const std::string& cmd);
    void recordWallFromCollision(const std::string& cmd);

    char readKey();
    void setupTerminal();
    void restoreTerminal();

    std::string server_ip_;
    unsigned short port_;
    std::string player_name_;
    bool perf_mode_;
    int socket_;
    int moves_left_;
    bool game_over_;
    std::string final_msg_;
    Maze maze_;
    std::vector<HistoryEntry> history_;
    termios saved_tty_;
    bool tty_saved_;
    char buffer_[1024];
};

} // namespace p2p_game

#endif // P2P_GAME_CLIENT_HPP
