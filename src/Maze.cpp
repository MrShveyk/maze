/** @file Maze.cpp
 *  Реализация модели лабиринта.
 */

#include "Maze.hpp"
#include "Utils.hpp"

#include <algorithm>
#include <string>
#include <utility>

namespace p2p_game {

Maze::Maze(unsigned size)
    : size_(size),
      player_x_(0),
      player_y_(0),
      goal_x_(static_cast<int>(size) - 1),
      goal_y_(static_cast<int>(size) - 1),
      gen_(std::random_device{}()) {}

void Maze::seed(std::uint32_t value) {
    gen_.seed(value);
}

Wall Maze::makeWall(Cell a, Cell b) {
    if (b < a) {
        std::swap(a, b);
    }
    return {a, b};
}

void Maze::setSize(unsigned size) {
    size_ = size;
    goal_x_ = static_cast<int>(size) - 1;
    goal_y_ = static_cast<int>(size) - 1;
    walls_.clear();
    player_x_ = 0;
    player_y_ = 0;
}

bool Maze::inBounds(int x, int y) const {
    const int s = static_cast<int>(size_);
    return x >= 0 && x < s && y >= 0 && y < s;
}

bool Maze::hasWallTop(int x, int y) const {
    return walls_.find(makeWall({x, y}, {x, y + 1})) != walls_.end();
}

bool Maze::hasWallRight(int x, int y) const {
    return walls_.find(makeWall({x, y}, {x + 1, y})) != walls_.end();
}

void Maze::addWall(int x1, int y1, int x2, int y2) {
    walls_.insert(makeWall({x1, y1}, {x2, y2}));
}

std::vector<Wall> Maze::allPossibleWalls() const {
    std::vector<Wall> result;
    const int s = static_cast<int>(size_);

    for (int y = 0; y < s; ++y) {
        for (int x = 0; x < s - 1; ++x) {
            result.push_back(makeWall({x, y}, {x + 1, y}));
        }
    }
    for (int x = 0; x < s; ++x) {
        for (int y = 0; y < s - 1; ++y) {
            result.push_back(makeWall({x, y}, {x, y + 1}));
        }
    }
    return result;
}

bool Maze::canMove(int dx, int dy) const {
    const int nx = player_x_ + dx;
    const int ny = player_y_ + dy;
    if (!inBounds(nx, ny)) {
        return false;
    }
    const Wall w = makeWall({player_x_, player_y_}, {nx, ny});
    return walls_.find(w) == walls_.end();
}

bool Maze::move(int dx, int dy) {
    if (!canMove(dx, dy)) {
        return false;
    }
    player_x_ += dx;
    player_y_ += dy;
    return true;
}

bool Maze::isWin() const {
    return player_x_ == goal_x_ && player_y_ == goal_y_;
}

void Maze::generate(unsigned wall_count) {
    walls_.clear();
    auto candidates = allPossibleWalls();
    if (candidates.empty()) {
        return;
    }

    std::shuffle(candidates.begin(), candidates.end(), gen_);

    unsigned added = 0;
    for (const auto& w : candidates) {
        if (added >= wall_count) {
            break;
        }
        walls_.insert(w);
        if (!pathExists()) {
            walls_.erase(w);
        } else {
            ++added;
        }
    }
}

bool Maze::pathExists() const {
    return dfsPathExists(walls_, static_cast<int>(size_), Cell(0, 0),
                         Cell(goal_x_, goal_y_));
}

void Maze::print(std::ostream& os) const {
    const int s = static_cast<int>(size_);

    os << "+";
    for (int x = 0; x < s; ++x) {
        os << "---+";
    }
    os << "\n";

    for (int y = s - 1; y >= 0; --y) {
        os << "|";
        for (int x = 0; x < s; ++x) {
            std::string content = "   ";
            if (x == player_x_ && y == player_y_) {
                content = " P ";
            } else if (x == goal_x_ && y == goal_y_) {
                content = " G ";
            }
            os << content;

            if (x < s - 1) {
                os << (hasWallRight(x, y) ? "|" : " ");
            } else {
                os << "|";
            }
        }
        os << "\n";

        if (y > 0) {
            os << "+";
            for (int x = 0; x < s; ++x) {
                os << (hasWallTop(x, y - 1) ? "---" : "   ");
                os << "+";
            }
            os << "\n";
        }
    }

    os << "+";
    for (int x = 0; x < s; ++x) {
        os << "---+";
    }
    os << "\n";
}

} // namespace p2p_game
