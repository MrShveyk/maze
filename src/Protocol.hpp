/** @file Protocol.hpp
 *  Описание прикладного протокола игры «Лабиринт» поверх TCP.
 *
 *  Протокол текстовый, удобный для отладки. Каждое сообщение завершается
 *  символом перевода строки `\n`. Сообщения нечувствительны к лишним
 *  пробелам — разбор выполняется по токенам (см. tokenize()).
 *
 *  Команды клиента:
 *  - `HELLO <имя>`   — подключение в обычном режиме (с лимитом ходов);
 *  - `PERF <имя>`    — подключение в режиме тестирования (без лимита);
 *  - `FORWARD`       — ход вперёд (+Y);
 *  - `BACK`          — ход назад (-Y);
 *  - `LEFT`          — ход влево (-X);
 *  - `RIGHT`         — ход вправо (+X);
 *  - `GIVEUP`        — сдаться.
 *
 *  Ответы сервера:
 *  - `SIZE <n>`              — размер поля (отправляется один раз после HELLO/PERF);
 *  - `OK <x> <y> <ходов>`    — успешный ход, новая позиция и остаток ходов;
 *  - `WALL <ходов>`          — наткнулись на стену, остаток ходов;
 *  - `WIN`                   — игрок достиг цели;
 *  - `LOSE`                  — игрок проиграл (исчерпал ходы или сдался);
 *  - `ERR <текст>`           — ошибка протокола.
 *
 *  После `WIN`/`LOSE`/`ERR` сервер закрывает соединение.
 */

#ifndef P2P_GAME_PROTOCOL_HPP
#define P2P_GAME_PROTOCOL_HPP

#include <sstream>
#include <string>
#include <vector>

namespace p2p_game {
namespace protocol {

// --- Запросы клиента ---
inline constexpr const char* REQ_HELLO   = "HELLO";
inline constexpr const char* REQ_PERF    = "PERF";
inline constexpr const char* REQ_FORWARD = "FORWARD";
inline constexpr const char* REQ_BACK    = "BACK";
inline constexpr const char* REQ_LEFT    = "LEFT";
inline constexpr const char* REQ_RIGHT   = "RIGHT";
inline constexpr const char* REQ_GIVEUP  = "GIVEUP";

// --- Ответы сервера ---
inline constexpr const char* RES_SIZE = "SIZE";
inline constexpr const char* RES_OK   = "OK";
inline constexpr const char* RES_WALL = "WALL";
inline constexpr const char* RES_WIN  = "WIN";
inline constexpr const char* RES_LOSE = "LOSE";
inline constexpr const char* RES_ERR  = "ERR";

/** Формирование запроса HELLO. */
inline std::string makeHello(const std::string& name) {
    return std::string(REQ_HELLO) + " " + name;
}

/** Формирование запроса PERF (режим тестирования). */
inline std::string makePerf(const std::string& name) {
    return std::string(REQ_PERF) + " " + name;
}

/** Формирование ответа SIZE. */
inline std::string makeSize(unsigned size) {
    return std::string(RES_SIZE) + " " + std::to_string(size);
}

/** Формирование ответа OK с координатами игрока и остатком ходов.
 *  Если ходы безлимитные, передаётся значение -1.
 */
inline std::string makeOk(int x, int y, int remaining) {
    return std::string(RES_OK) + " " + std::to_string(x) + " " +
           std::to_string(y) + " " + std::to_string(remaining);
}

/** Формирование ответа WALL с остатком ходов. */
inline std::string makeWall(int remaining) {
    return std::string(RES_WALL) + " " + std::to_string(remaining);
}

/** Формирование сообщения об ошибке протокола. */
inline std::string makeErr(const std::string& reason) {
    return std::string(RES_ERR) + " " + reason;
}

/** Разбиение строки на токены по пробелам. */
inline std::vector<std::string> tokenize(const std::string& msg) {
    std::vector<std::string> tokens;
    std::istringstream iss(msg);
    std::string tok;
    while (iss >> tok) {
        tokens.push_back(tok);
    }
    return tokens;
}

} // namespace protocol
} // namespace p2p_game

#endif // P2P_GAME_PROTOCOL_HPP
