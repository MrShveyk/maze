/** @file Maze.hpp
 *  Модель игрового поля «Лабиринт».
 *
 *  @details Лабиринт представляется квадратным полем заданного размера.
 *  Игрок стартует в левом нижнем углу (0, 0), цель находится в правом
 *  верхнем углу (size-1, size-1). Стены располагаются между соседними
 *  клетками и хранятся в виде упорядоченного множества пар клеток.
 *
 *  Класс используется как на сервере (генерация и логика игры), так и
 *  на клиенте (локальное отображение известных стен).
 */

#ifndef P2P_GAME_MAZE_HPP
#define P2P_GAME_MAZE_HPP

#include <cstdint>
#include <ostream>
#include <random>
#include <set>
#include <vector>

namespace p2p_game {

/** Клетка игрового поля. */
struct Cell {
    int x = 0;
    int y = 0;

    Cell() = default;
    Cell(int x_, int y_) : x(x_), y(y_) {}

    bool operator<(const Cell& other) const {
        if (x != other.x) {
            return x < other.x;
        }
        return y < other.y;
    }

    bool operator==(const Cell& other) const {
        return x == other.x && y == other.y;
    }
};

/** Стена между двумя соседними клетками.
 *
 *  Стена представляет собой неупорядоченную пару клеток. Чтобы обеспечить
 *  однозначное хранение в std::set, ребро всегда нормализуется так, что
 *  `from < to` (см. Maze::makeWall).
 */
struct Wall {
    Cell from;
    Cell to;

    bool operator<(const Wall& other) const {
        if (from < other.from) return true;
        if (other.from < from) return false;
        return to < other.to;
    }
};

/** Модель лабиринта.
 *
 *  Хранит размер поля, множество стен и текущую позицию игрока. Содержит
 *  методы для случайной генерации стен с гарантией существования пути от
 *  старта до цели, проверки возможности перемещения, выполнения хода и
 *  текстового отображения поля.
 *
 *  Выбор структур данных:
 *  - `std::set<Wall>` обеспечивает O(log N) проверку наличия стены и
 *    исключает дубликаты;
 *  - нормализация ребра (Maze::makeWall) делает множество не зависящим
 *    от порядка перечисления вершин;
 *  - локальный `std::mt19937` инкапсулирует генератор случайных чисел и
 *    позволяет независимо тестировать сценарии.
 */
class Maze {
public:
    /** Создаёт поле заданного размера. */
    explicit Maze(unsigned size = 3u);

    /** Переинициализирует ГСЧ заданным seed.
     *  Используется для воспроизводимой генерации лабиринта.
     */
    void seed(std::uint32_t value);

    /** Случайно расставляет стены с сохранением достижимости цели.
     *  @param wall_count желаемое число стен.
     */
    void generate(unsigned wall_count);

    /** Проверяет возможность хода без выполнения. */
    bool canMove(int dx, int dy) const;

    /** Пытается выполнить ход. Возвращает true, если перемещение удалось. */
    bool move(int dx, int dy);

    /** Достиг ли игрок целевой клетки. */
    bool isWin() const;

    int playerX() const { return player_x_; }
    int playerY() const { return player_y_; }
    int goalX() const { return goal_x_; }
    int goalY() const { return goal_y_; }
    unsigned size() const { return size_; }

    /** Печать поля для консоли клиента. */
    void print(std::ostream& os) const;

    /** Нормализованный конструктор стены: from всегда «меньше» to. */
    static Wall makeWall(Cell a, Cell b);

    /** Существует ли путь от старта до цели (DFS). */
    bool pathExists() const;

    /** Принудительная установка позиции игрока (для клиента). */
    void setPlayerPosition(int x, int y) {
        player_x_ = x;
        player_y_ = y;
    }

    /** Добавление стены между (x1, y1) и (x2, y2) — используется клиентом
     *  при получении ответа WALL для запоминания препятствия.
     */
    void addWall(int x1, int y1, int x2, int y2);

    /** Установка размера поля (используется клиентом по ответу SIZE). */
    void setSize(unsigned size);

private:
    bool hasWallTop(int x, int y) const;
    bool hasWallRight(int x, int y) const;
    bool inBounds(int x, int y) const;

    /** Возвращает все возможные рёбра между соседними клетками. */
    std::vector<Wall> allPossibleWalls() const;

    unsigned size_;
    int player_x_;
    int player_y_;
    int goal_x_;
    int goal_y_;
    std::set<Wall> walls_;
    std::mt19937 gen_;
};

} // namespace p2p_game

#endif // P2P_GAME_MAZE_HPP
