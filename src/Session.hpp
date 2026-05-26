/** @file Session.hpp
 *  Игровая сессия сервера — обработка одного клиента в дочернем процессе.
 *
 *  Объект Session создаётся в дочернем процессе после fork() сервера и
 *  выполняет полный цикл взаимодействия: приём HELLO/PERF, генерация
 *  лабиринта, обработка ходов, отправка ответов и закрытие соединения.
 */

#ifndef P2P_GAME_SESSION_HPP
#define P2P_GAME_SESSION_HPP

#include <string>
#include <vector>

#include "Maze.hpp"

namespace p2p_game {

/** Конфигурация сессии. */
struct SessionConfig {
    int max_moves = 10;     ///< Лимит ходов в обычном режиме.
    unsigned maze_size = 3; ///< Размер стороны квадратного поля.
    unsigned wall_count = 6; ///< Число генерируемых стен.
};

/** Игровая сессия: всё взаимодействие с одним клиентом. */
class Session {
public:
    Session(int client_socket, const SessionConfig& cfg);
    ~Session();

    Session(const Session&) = delete;
    Session& operator=(const Session&) = delete;

    /** Основной цикл сессии. Возвращает по завершении игры. */
    void run();

private:
    /** Чтение одного сообщения, оканчивающегося '\n'.
     *  @return текст сообщения без терминатора.
     *  @throw std::runtime_error при разрыве соединения.
     */
    std::string recvLine();

    /** Отправка сообщения с добавлением '\n'. */
    void sendLine(const std::string& msg);

    /** Приветственное сообщение (HELLO / PERF). */
    void handleInit(const std::vector<std::string>& tokens);

    /** Обработка хода. Возвращает текст ответа для клиента. */
    std::string handleCommand(const std::string& cmd);

    /** Преобразование команды в смещение по координатам. */
    static bool cmdToDelta(const std::string& cmd, int& dx, int& dy);

    int socket_;
    SessionConfig cfg_;
    int moves_left_;
    bool perf_mode_;
    Maze maze_;
    std::string player_name_;
    char buffer_[1024];
};

} // namespace p2p_game

#endif // P2P_GAME_SESSION_HPP
