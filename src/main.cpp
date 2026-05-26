/** @file main.cpp
 *  Точка входа приложения «Лабиринт».
 *
 *  Один и тот же бинарник запускается либо в режиме сервера (`-s`),
 *  либо в режиме клиента (`-c`). Выбор режима происходит без
 *  перекомпиляции — в зависимости от флага командной строки.
 */

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

#include <unistd.h>

#include "Client.hpp"
#include "Server.hpp"

namespace {

void printUsage(const char* prog, std::ostream& os = std::cerr) {
    os << "Использование:\n"
       << "  Сервер:  " << prog
       << " -s [-p порт] [-m лимит_ходов] [-z размер] [-w стен]\n"
       << "    -p порт          порт прослушивания (по умолчанию 4321)\n"
       << "    -m лимит_ходов   число ходов в обычном режиме (по умолчанию 10)\n"
       << "    -z размер        размер стороны лабиринта (по умолчанию 3)\n"
       << "    -w стен          число стен (по умолчанию size*(size-1))\n"
       << "\n"
       << "  Клиент:  " << prog
       << " -c -n имя [-a адрес] [-p порт] [-t]\n"
       << "    -n имя     имя игрока (обязательно)\n"
       << "    -a адрес   IP-адрес сервера (по умолчанию 127.0.0.1)\n"
       << "    -p порт    порт сервера (по умолчанию 4321)\n"
       << "    -t         режим тестирования производительности\n"
       << "\n"
       << "  Справка: " << prog << " -h\n";
}

int parsePort(const char* arg, unsigned short& out) {
    const int v = std::atoi(arg);
    if (v <= 0 || v > 65535) {
        std::cerr << "Ошибка: порт должен быть в диапазоне 1..65535\n";
        return 1;
    }
    out = static_cast<unsigned short>(v);
    return 0;
}

int runServer(int argc, char** argv) {
    p2p_game::SessionConfig cfg;
    unsigned short port = 4321;
    bool wall_count_set = false;

    ::optind = 1;
    int opt;
    while ((opt = getopt(argc, argv, "scp:m:z:w:n:a:th")) != -1) {
        switch (opt) {
            case 'p':
                if (parsePort(optarg, port) != 0) return 1;
                break;
            case 'm': {
                const int v = std::atoi(optarg);
                if (v <= 0) {
                    std::cerr << "Ошибка: лимит ходов должен быть положительным\n";
                    return 1;
                }
                cfg.max_moves = v;
                break;
            }
            case 'z': {
                const int v = std::atoi(optarg);
                if (v < 2 || v > 16) {
                    std::cerr << "Ошибка: размер лабиринта должен быть 2..16\n";
                    return 1;
                }
                cfg.maze_size = static_cast<unsigned>(v);
                break;
            }
            case 'w': {
                const int v = std::atoi(optarg);
                if (v < 0) {
                    std::cerr << "Ошибка: число стен не может быть отрицательным\n";
                    return 1;
                }
                cfg.wall_count = static_cast<unsigned>(v);
                wall_count_set = true;
                break;
            }
            case 's':
            case 'c':
            case 'n':
            case 'a':
            case 't':
                break;
            case 'h':
            default:
                return 1;
        }
    }

    if (!wall_count_set) {
        cfg.wall_count = cfg.maze_size * (cfg.maze_size - 1);
    }
    const unsigned max_walls = 2u * cfg.maze_size * (cfg.maze_size - 1u);
    if (cfg.wall_count > max_walls) {
        std::cerr << "Ошибка: число стен превышает максимум (" << max_walls << ")\n";
        return 1;
    }

    try {
        p2p_game::Server server(port, cfg);
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка сервера: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

int runClient(int argc, char** argv) {
    std::string server_ip = "127.0.0.1";
    unsigned short port = 4321;
    std::string player_name;
    bool perf_mode = false;

    ::optind = 1;
    int opt;
    while ((opt = getopt(argc, argv, "scp:m:z:w:n:a:th")) != -1) {
        switch (opt) {
            case 'a':
                server_ip = optarg;
                break;
            case 'p':
                if (parsePort(optarg, port) != 0) return 1;
                break;
            case 'n':
                player_name = optarg;
                break;
            case 't':
                perf_mode = true;
                break;
            case 'c':
            case 's':
            case 'm':
            case 'z':
            case 'w':
                break;
            case 'h':
            default:
                return 1;
        }
    }

    if (player_name.empty()) {
        std::cerr << "Ошибка: укажите имя игрока (-n)\n";
        return 1;
    }

    try {
        p2p_game::Client client(server_ip, port, player_name, perf_mode);
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Ошибка клиента: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    bool is_server = false;
    bool is_client = false;
    bool is_help = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg(argv[i]);
        if (arg == "-s") is_server = true;
        else if (arg == "-c") is_client = true;
        else if (arg == "-h" || arg == "--help") is_help = true;
    }

    if (is_help || (!is_server && !is_client)) {
        printUsage(argv[0], is_help ? std::cout : std::cerr);
        return is_help ? 0 : 1;
    }
    if (is_server && is_client) {
        std::cerr << "Ошибка: укажите только один режим (-s или -c)\n";
        return 1;
    }

    return is_server ? runServer(argc, argv) : runClient(argc, argv);
}
