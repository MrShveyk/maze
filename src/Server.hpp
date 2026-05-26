/** @file Server.hpp
 *  Многопроцессный TCP-сервер игры «Лабиринт».
 *
 *  Реализация в соответствии с вариантом 2.1: для каждого подключения
 *  выполняется fork(), и игровая сессия с клиентом проходит в отдельном
 *  дочернем процессе. Родительский процесс продолжает принимать новые
 *  подключения. Завершившиеся дочерние процессы собираются через
 *  обработчик SIGCHLD, чтобы не оставалось zombie.
 */

#ifndef P2P_GAME_SERVER_HPP
#define P2P_GAME_SERVER_HPP

#include "Session.hpp"

namespace p2p_game {

/** Многопроцессный TCP-сервер. */
class Server {
public:
    Server(unsigned short port, const SessionConfig& cfg);
    ~Server();

    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /** Главный цикл: accept → fork → Session::run. */
    void run();

private:
    /** Создание, настройка и перевод сокета в режим прослушивания. */
    void setupSocket();

    /** Обработчик SIGCHLD: сбор всех завершившихся дочерних процессов. */
    static void onSigChld(int);

    unsigned short port_;
    SessionConfig cfg_;
    int listen_socket_;
};

} // namespace p2p_game

#endif // P2P_GAME_SERVER_HPP
