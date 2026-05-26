/** @file Utils.hpp
 *  Вспомогательные алгоритмы (поиск пути в лабиринте).
 */

#ifndef P2P_GAME_UTILS_HPP
#define P2P_GAME_UTILS_HPP

#include <set>

#include "Maze.hpp"

namespace p2p_game {

/** Проверяет существование пути от @p start к @p goal на поле размера @p size
 *  при заданном множестве стен (обход в глубину на стеке).
 */
bool dfsPathExists(const std::set<Wall>& walls,
                   int size,
                   const Cell& start,
                   const Cell& goal);

} // namespace p2p_game

#endif // P2P_GAME_UTILS_HPP
