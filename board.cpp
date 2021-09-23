#include "board.h"

Board::Board(int size)
    : size(size)
{
    init(size);
}

bool Board::init(int size)
{
    if (size < 1 || size > MAX_BOARD_SIZE) {
        return false;
    }

    this->size = size;

    for (int x = 0; x < CELLS_SIZE; x++) {
        for (int y = 0; y < CELLS_SIZE; y++) {
            if (x > 0 && x <= size && y > 0 && y <= size) {
                cells[x][y] = CELL_SPACE;
            } else {
                cells[x][y] = CELL_OUT;
            }
        }
    }

    kifu.clear();
    cur = Point();
    last = Point();
    kou = Point();

    is_pass = false;
    pass_stone = CELL_SPACE;
    n_moves = 0;
    n_black_hama = 0;
    n_white_hama = 0;

    return true;
}

int Board::get_size() const
{
    return size;
}

bool Board::is_empty() const
{
    for (int x = 1; x <= size; x++) {
        for (int y = 1; y <= size; y++) {
            if (cells[x][y] != CELL_SPACE)
                return false;
        }
    }

    return true;
}

bool Move::operator<(const Move &m) const {
    int a = this->cell * (CELLS_SIZE*CELLS_SIZE) + this->x + this->y * CELLS_SIZE;
    int b = m.cell * (CELLS_SIZE*CELLS_SIZE) + m.x + m.y * CELLS_SIZE;
    return a < b;
}

std::ostream& operator<<(std::ostream& os, const Board& board)
{
    os << "kou: " << board.get_kou() << std::endl;

    if (board.kifu.size() != 0) {
        Move last = board.kifu.back();
        char stone;
        if ((last.cell & 3) == CELL_BLACK) {
            stone = 'b';
        } else {
            stone = 'w';
        }
        os << "last: " << stone << " (" << last.x << ", " << last.y << ")" << std::endl;
    }

    for (int y = 1; y <= board.size; y++) {
        for (int x = 1; x <= board.size; x++) {
            int v = board.cells[x][y] & 3;

            if (v == CELL_BLACK) {
                os << "*";
            } else if (v == CELL_WHITE) {
                os << "o";
            } else {
                os << "+";
            }
        }

        os << std::endl;
    }

    return os;
}

// 戻り値は CELL_SPACE, CELL_BLACK, CELL_WHITE, CELL_OUT に限る。
// ただし、これらが足された組み合わせなど変な値が設定されていないことが前提
int Board::get_val(int x, int y) const
{
    if (is_out(x, y)) {
        return CELL_OUT;
    }

    return cells[x][y] & 7;
}

int Board::get_cell(int x, int y) const
{
    if (is_out(x, y)) {
        return CELL_OUT;
    }

    return cells[x][y];
}

int Board::set_cell(int cell, int x, int y)
{
    if (is_out(x, y)) {
        return CELL_OUT;
    }

    int old = cells[x][y];

    cells[x][y] = cell;

    return old;
}

int Board::set_flag(int flag, int x, int y)
{
    if (is_out(x, y)) {
        return CELL_OUT;
    }

    int old = cells[x][y];

    cells[x][y] |= flag;

    return old;
}

int Board::clear_flag(int x, int y)
{
    if (is_out(x, y)) {
        return CELL_OUT;
    }

    int old = cells[x][y];

    cells[x][y] &= 7;

    return old;
}

bool Board::has_stone(int x, int y) const
{
    if (is_out(x, y)) {
        return false;
    }

    return cells[x][y] & 3;
}

bool Board::has_stone(const Point& pt) const
{
    return has_stone(pt.x, pt.y);
}

inline bool Board::is_out(int x, int y) const
{
    if (x < 1 || x > size || y < 1 || y > size) {
        return true;
    }

    return false;
}

bool Board::is_kou(int x, int y)
{
    return kou == Point(x, y);
}

void Board::set_kou(const Point& pt)
{
    if (is_out(pt.x, pt.y)) {
        kou.set(0, 0);
    }

    kou = pt;
}

Point Board::get_kou() const
{
    return kou;
}

void Board::set_cur(int x, int y)
{
    if (is_out(x, y)) {
        cur.set(0, 0);
    }

    cur.set(x, y);
}

Point Board::get_cur() const
{
    return cur;
}

void Board::set_last(const Point& pt)
{
    if (is_out(pt.x, pt.y)) {
        last.set(0, 0);
    }

    last = pt;
}

Point Board::get_last() const
{
    return last;
}

int Board::get_last_cell() const
{
    if (last.x == 0) {
        return CELL_SPACE;
    }

    if (last.x == 20 && last.y == 20) {
        return pass_stone;
    }

    return get_val(last.x, last.y);
}


static inline int get_opponent(int stone)
{
    if (stone == CELL_BLACK) {
        return CELL_WHITE;
    }

    return CELL_BLACK;
}

// right, up, left, down
static std::array<Point, 4> ruld = {Point(1, 0), Point(0, 1), Point(-1, 0), Point(0, -1)};

bool Board::make_move(int cell, int x, int y, std::list<Move>& hama)
{
    // pass
    if (x == 20 && y == 20) {
        is_pass = true;
        pass_stone = cell & 3;
        n_moves++;
        kifu.push_back(Move(cell & 3, x, y));
        set_last(Point(x, y));
        return true;
    }

    int val = cell & 3;

    // 石じゃなかったり、範囲外だったりしないか？
    if (val == 0 || is_out(x, y)) {
        return false;
    }

    // すでに石が置かれている場合
    if (has_stone(x, y)) {
        return false;
    }

    if (is_kou(x, y)) {
        // コウのため着手禁止
        return false;
    }

    int old_cell = get_cell(x, y);
    set_cell(val, x, y);  // 仮置き

    // コウの可能性を確認する
    bool kou_kamo = true;
    for (Point pt : ruld) {
        int v = get_val(x + pt.x, y + pt.y);

        if (v == val || v == CELL_SPACE) {
            kou_kamo = false;
        }
    }

    // 石が敵に囲まれているか？
    std::map<std::string, bool> memo;
    if (is_surrounded(memo, val, x, y)) {
        int opponent = get_opponent(val);
        bool can_kill = false;

        for (Point pt : ruld) {
            if (can_kill == false and get_val(x + pt.x, y + pt.y) == opponent) {
                memo.clear();
                if (is_surrounded(memo, opponent, x + pt.x, y + pt.y)) {
                    can_kill = true;
                }
            }
        }

        if (can_kill == false) {
            // 自殺のため着手禁止
            set_cell(old_cell, x, y); // 仮置きした石をもとに戻す
            return false;
        }
    }

    // 石を置ける

    n_moves++;

    kifu.push_back(Move(val, x, y));

    // 敵を取れるなら取る

    is_pass = false;
    pass_stone = CELL_SPACE;
    int cnt = 0;  // トータルで取った石
    Point kou = Point();

    for (Point pt : ruld) {
        int c = take_prisoners_if_ok(val, x + pt.x, y + pt.y, hama);

        cnt += c;

        if (c == 1) {
            kou.set(x + pt.x, y + pt.y);
        }
    }

    if (val == CELL_BLACK) {
        n_black_hama += cnt;
    } else {
        n_white_hama += cnt;
    }

    if (kou_kamo == false or cnt != 1) {
        // コウじゃない
        kou.set(0, 0);
    }

    set_kou(kou);
    set_last(Point(x, y));

    return true;
}

static std::string make_key(int x, int y)
{
    std::string s = std::to_string(x);
    s += "x";
    s += std::to_string(y);

    return s;
}

// 敵に囲まれているか調べる
bool Board::is_surrounded(std::map<std::string, bool>& memo, int my_stone, int x, int y)
{
    std::string key = make_key(x, y);

    // すでに調べた位置か？
    std::map<std::string, bool>::iterator itr = memo.find(key);
    if (itr != memo.end()) {
        return true;
    }

    memo[key] = true;

    int stone = get_val(x, y);

    if (stone == CELL_SPACE) {
        return false;
    }

    if (my_stone == stone) {
        return is_surrounded(memo, my_stone, x+1, y) &&
               is_surrounded(memo, my_stone, x, y+1) &&
               is_surrounded(memo, my_stone, x-1, y) &&
               is_surrounded(memo, my_stone, x, y-1);
    }

    // 敵か盤外
    return true;
}

// 敵を取れるなら取る
int Board::take_prisoners_if_ok(int my_stone, int x, int y, std::list<Move>& hama)
{
    int opponent = get_opponent(my_stone);

    // 敵でないなら取らない
    if (get_val(x, y) != opponent) {
        return 0;
    }

    std::map<std::string, bool> memo;
    if (is_surrounded(memo, opponent, x, y)) {
        return take_prisoners(opponent, x, y, hama);
    }

    return 0;
}

// (x, y) の石と、連なった石を取り除く
int Board::take_prisoners(int stone, int x, int y, std::list<Move>& hama)
{
    if (get_val(x, y) != stone) {
        return 0;
    }

    Move v(get_cell(x, y), x, y);
    hama.push_back(v);

    set_cell(CELL_SPACE, x, y);

    return (1 +
            take_prisoners(stone, x+1, y, hama) +
            take_prisoners(stone, x, y+1, hama) +
            take_prisoners(stone, x-1, y, hama) +
            take_prisoners(stone, x, y-1, hama));
}
