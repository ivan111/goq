#ifndef BOARD_H
#define BOARD_H

#include <iostream>
#include <list>
#include <map>
#include <vector>

#define MAX_BOARD_SIZE 19
#define CELLS_SIZE (MAX_BOARD_SIZE+2)

enum {
    CELL_SPACE = 0x00,
    CELL_BLACK = 0x01,
    CELL_WHITE = 0x02,
    CELL_OUT   = 0x04,

    // markup
    CELL_MA = 0x10,  // X
    CELL_TR = 0x20,  // triangle
    CELL_CR = 0x40,  // circle

    CELL_CORRECT = 0x100,
    CELL_WRONG = 0x200,
};

#define LBL_MASK (0xffffff000)
#define LBL_SHIFT (12)

struct Point
{
    int x, y;
    Point() : x(0), y(0) {};
    Point(int x, int y) : x(x), y(y) {};
    void set(int x, int y) { this->x = x; this->y = y; };

    bool operator==(const Point& pt) const { return x == pt.x && y == pt.y; };
    bool operator!=(const Point& pt) const { return !(*this == pt); };
    operator bool() const { return x != 0 && y != 0; };

    friend std::ostream& operator<<(std::ostream& os, const Point& pt) {
        return os << "(" << pt.x << ", " << pt.y << ")";
    };

    bool operator<(const Point &pt) const {
        return this->x + this->y * CELLS_SIZE < pt.x + pt.y * CELLS_SIZE;
    };
};


struct Move
{
    int cell, x, y;

    Move() : cell(0), x(0), y(0) {};
    Move(int cell, int x, int y) : cell(cell), x(x), y(y) {};

    bool operator==(const Move& m) const {
        return cell == m.cell && x == m.x && y == m.y;
    };
    bool operator!=(const Move& m) const { return !(*this == m); };

    bool operator<(const Move &m) const;
};

Move rotate(int n, int size, Move m);
Move flip(int n, Move m);

class Board
{
    int size;
    int cells[CELLS_SIZE][CELLS_SIZE];
    std::vector<Move> kifu;  // ex. kifu[0] == [CELL_BLACK, x, y]
    Point cur = Point();  // 現在位置が設定されていないとき {0, 0}
    Point last = Point();  // 最後の位置が設定されていないとき {0, 0}
    Point kou = Point();  // コウじゃないとき {0, 0}
public:
    Board(int size);

    bool init(int size);

    friend std::ostream& operator<<(std::ostream& os, const Board& board);

    bool is_empty() const;

    int get_size() const;
    int get_val(int x, int y) const;
    int get_cell(int x, int y) const;
    int set_cell(int cell, int x, int y);

    int set_flag(int flag, int x, int y);
    int clear_flag(int x, int y);

    bool is_out(int x, int y) const;
    bool has_stone(int x, int y) const;
    bool has_stone(const Point& pt) const;

    void set_cur(int x, int y);
    Point get_cur() const;

    void set_last(const Point& pt);
    Point get_last() const;
    int get_last_cell() const;

    bool is_kou(int x, int y);
    void set_kou(const Point& pt);
    Point get_kou() const;

    bool is_pass = false;
    int pass_stone = CELL_SPACE;
    int n_moves = 0;  // 手数
    int n_black_hama = 0;
    int n_white_hama = 0;

    std::vector<Move>& get_kifu() { return kifu; };

    bool make_move(int cell, int x, int y, std::list<Move>& hama);
    bool is_surrounded(std::map<std::string, bool>& memo, int my_stone, int x, int y);
    int take_prisoners_if_ok(int my_stone, int x, int y, std::list<Move>& hama);
    int take_prisoners(int stone, int x, int y, std::list<Move>& hama);
};

#endif
