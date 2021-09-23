#include <algorithm>
#include <cassert>
#include <random>
#include <sstream>
#include "g.h"
#include "gio.h"
#include "command.h"

Route::Route(std::shared_ptr<Node> root)
{
    m_route.push_back(root);
    m_idx = 0;
    m_min_idx = 0;
}

std::shared_ptr<Node> Route::current()
{
    return m_route[m_idx];
}

std::shared_ptr<Node> Route::next()
{
    if (m_idx + 1 < (int)m_route.size()) {
        return m_route[m_idx+1];
    }

    std::shared_ptr<Node> none;
    return none;
}

bool Route::select(std::shared_ptr<Node> node)
{
    std::shared_ptr<Node> cur = current();

    bool exists = false;

    // 現在のノードの子じゃないとダメ
    for (auto n : cur->children) {
        if (n == node) {
            exists = true;
            break;
        }
    }

    assert(exists);

    if (!exists) {
        return false;
    }

    remove_nodes_after_cur();

    m_route.push_back(node);
    m_idx++;

    return true;
}

bool Route::visit_parent()
{
    if (m_idx < 1) {
        return false;
    }

    m_idx--;
    return true;
}

void Route::visit_min_undo()
{
    m_idx = m_min_idx;
}

bool Route::has_min_undo_children()
{
    std::shared_ptr<Node> n = m_route[m_min_idx];

    return !n->children.empty();
}

bool Route::exists(std::shared_ptr<Node> node)
{
    for (auto n : m_route) {
        if (n == node) {
            return true;
        }
    }

    return false;
}

void Route::set_min_undo()
{
    m_min_idx = m_idx;
}

void Route::clear_min_undo()
{
    m_min_idx = 0;
}

int Route::remove_nodes_after_cur()
{
    int num_remove = m_route.size() - m_idx - 1;

    for (int i = 0; i < num_remove; i++) {
        m_route.pop_back();
    }

    return num_remove;
}

void Route::append(std::shared_ptr<Node> node, bool do_select)
{
    std::shared_ptr<Node> cur = current();
    cur->children.push_back(node);

    if (do_select) {
        select(node);
    }
}

bool Route::can_undo()
{
    return m_min_idx < m_idx;
}

bool Route::undo(G& g)
{
    if (!can_undo()) {
        return false;
    }

    std::shared_ptr<Node> cur = current();
    cur->undo(g);
    m_idx--;

    return true;
}

bool Route::can_redo()
{
    return m_idx + 1 < (int)m_route.size();
}

bool Route::redo(G& g)
{
    if (!can_redo()) {
        return false;
    }

    m_idx++;
    std::shared_ptr<Node> cur = current();
    cur->redo(g);

    return true;
}

void Route::redo_history(G& g)
{
    for (int i = 0; i <= m_idx; i++) {
        std::shared_ptr<Node> node = m_route[i];
        node->redo(g);
    }
}

std::ostream& operator<<(std::ostream& os, const Route& obj)
{
    std::shared_ptr<Node> cur = obj.m_route[obj.m_idx];

    for (int i = 1; i < (int)obj.m_route.size(); i++) {
        std::shared_ptr<Node> node = obj.m_route[i];

        if (node == cur) {
            os << '*';
        }

        os << *node << std::endl;
    }

    return os;
}

#define DEFAULT_BOARD_SIZE (13)

Game::Game(G& g, Gmode mode, std::shared_ptr<Node> root, int size)
    : g(g), mode(mode), root(root), route(root), size(size)
{
}

static std::string ID_CORRECT = "CORRECT";
static std::string ID_WRONG = "WRONG";

void tr_set_correct_path(std::shared_ptr<Node> node, std::list<std::shared_ptr<Node>> route)
{
    for (auto p : node->properties) {
        if (p->is_correct()) {
            for (auto prop : node->properties) {
                if (prop->is_move()) {
                    Point pt = prop->point();
                    node->properties.push_back(create_property(ID_CORRECT, pt.x, pt.y));
                    break;
                }
            }

            for (auto n : route) {
                n->set_correct_path(true);
            }
            return;
        }
    }
}

void tr_set_wrong_mark(std::shared_ptr<Node> node, std::list<std::shared_ptr<Node>> route)
{
    (void)route;

    if (node->is_correct_path()) {
        for (auto n : node->children) {
            if (!n->is_correct_path()) {
                for (auto p : n->properties) {
                    if (p->is_move()) {
                        Point pt = p->point();
                        n->properties.push_back(create_property(ID_WRONG, pt.x, pt.y));
                        break;
                    }
                }
            }
        }
    }
}

void Game::set_correct_path()
{
    std::list<std::shared_ptr<Node>> lst;
    traverse(root, lst, tr_set_correct_path);
}

void Game::set_wrong_mark()
{
    std::list<std::shared_ptr<Node>> lst;
    traverse(root, lst, tr_set_wrong_mark);
}

void Game::setup()
{
    if (mode == Gmode::SOLVE) {
        set_correct_path();
        set_wrong_mark();
    }

    std::list<std::shared_ptr<Node>> lst;
    node_to_list_main_path(lst, root);

    lst.pop_front();  // rootはすでに追加されているので除去
    root->exec(g);

    for (auto p : root->properties) {
        PID pid = p->pid();

        if (pid == PID::PB) {
            player_black = p->val();
        } else if (pid == PID::PW) {
            player_white = p->val();
        }
    }

    for (auto n : lst) {
        bool done = n->exec(g);

        route.select(n);

        if (done) {
            break;
        }
    }

    for (auto n : lst) {
        n->set_next_move();
    }

    set_my_stone();

    route.set_min_undo();

    g.dispatch_info_event();
}

std::string Game::get_sgf()
{
    std::string s = "(";

    s += root->to_sgf();
    s += ")";

    return s;
}


bool Game::set_my_stone()
{
    my_stone = CELL_SPACE;

    std::shared_ptr<Node> cur = route.current();

    for (auto prop : cur->properties) {
        if (prop->pid() == PID::PL) {
            if (prop->pid() == PID::B) {
                my_stone = CELL_BLACK;
                return true;
            } else if (prop->pid() == PID::W) {
                my_stone = CELL_WHITE;
                return true;
            } else {
                break;
            }
        }
    }

    for (auto p : cur->get_next_move()) {
        std::shared_ptr<Node> n = p.second;

        for (auto prop : n->properties) {
            if (prop->pid() == PID::B) {
                my_stone = CELL_BLACK;
                return true;
            } else if (prop->pid() == PID::W) {
                my_stone = CELL_WHITE;
                return true;
            }
        }
    }

    return false;
}

void Game::print_nodes()
{
    if (root.use_count() == 0) {
        return;
    }

    std::shared_ptr<Node> cur = route.current();

    std::list<std::shared_ptr<Node>> lst;
    node_to_list(lst, root);

    for (auto n : lst) {
        if (cur == n) {
            std::cout << '*';
        }

        std::cout << *n << std::endl;
    }
}

void Game::print_route()
{
    std::cout << route << std::endl;
}

static char place_stone_id[3][3] = {"AE", "AB", "AW"};
static char make_move_id[3][2] = {"?", "B", "W"};

bool Game::do_put_stone(int cell, int x, int y, bool is_move, bool is_wrong)
{
    std::string id;

    if (is_move == true && cell >= 1 && cell < 3) {
        id = make_move_id[cell];
    } else if (is_move == false && cell >= 0 && cell < 3) {
        id = place_stone_id[cell];
    } else {
        return false;
    }

    std::shared_ptr<Node> node = create_node(id, x, y);

    if (node.use_count() == 0) {
        return false;
    }

    if (is_wrong) {
        node->properties.push_back(create_property(ID_WRONG, x, y));
    }

    if (!node->exec(g)) {
        return false;
    }

    if (mode == Gmode::KIFU && !is_auto_increment()) {
        set_auto_increment(g.board.get_kifu().size() - 1);
    }

    remove_no_use_history();
    route.append(node);

    g.dispatch_tree_event();
    g.dispatch_info_event();

    return true;
}

bool Game::place_stone(int cell, int x, int y)
{
    std::string id;

    if (cell >= 0 && cell < 3) {
        id = place_stone_id[cell];
    } else {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();

    if (cell == CELL_SPACE) { // 削除
        cur->remove_property_val(x, y);
    } else {
        if (!cur->merge_property(id, x, y)) {
            return false;
        }
    }

    g.board.set_cell(cell, x, y);

    g.dispatch_tree_event();

    return true;
}

std::shared_ptr<Node> Game::search_next_node(int cell, int x, int y)
{
    std::shared_ptr<Node> cur = route.current();

    Move m = Move(cell, x, y);

    std::shared_ptr<Node> next;

    for (auto p : cur->get_next_move()) {
        if (p.first == m) {
            next = p.second;
            break;
        }
    }

    return next;
}

bool Game::do_make_solve_move(int cell, int x, int y)
{
    if (cell != my_stone) {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();

    if (cur->get_next_move().empty()) {
        return false;
    }

    std::shared_ptr<Node> next = search_next_node(cell, x, y);

    if (next.use_count() == 0) {
        return do_put_stone(cell, x, y, true, true);
    }

    remove_no_use_history();
    next->exec(g);
    route.select(next);

    cur = route.current();

    // 相手の次の手があるなら実行する
    PID pid;
    if (my_stone == CELL_BLACK) {
        pid = PID::W;
    } else {
        pid = PID::B;
    }

    for (auto child : cur->children) {
        if (child->has(pid)) {
            child->exec(g);
            route.select(child);
            break;
        }
    }

    g.dispatch_tree_event();
    g.dispatch_info_event();

    return true;
}

void Game::remove_no_use_history()
{
    std::shared_ptr<Node> cur = route.current();

    // routeから辿れるnodeの中でいらないものを削除
    if (mode == Gmode::CREATE || mode == Gmode::SOLVE) {
        std::shared_ptr<Node> next = route.next();
        cur->remove_child(next);
    }

    // routeからいらないものを削除
    route.remove_nodes_after_cur();
}

bool Game::put_stone(int cell, int x, int y)
{
    if (x == 0) {
        return false;
    }

    switch (mode) {
    case Gmode::CREATE:
        return place_stone(cell, x, y);

    case Gmode::ANSWER:
    case Gmode::FREE:
    case Gmode::KIFU:
    {
        // 白黒交互に打つ
        if (cell == g.board.get_last_cell()) {
            return false;
        }

        // すでに同じ手があるなら探す
        PID pid;
        if (cell & CELL_BLACK) {
            pid = PID::B;
        } else if (cell & CELL_WHITE) {
            pid = PID::W;
        } else {
            return false;
        }

        Point pt = Point(x, y);

        std::shared_ptr<Node> cur = route.current();
        for (auto n : cur->children) {
            for (auto p : n->properties) {
                if (p->pid() == pid && p->point() == pt) {
                    route.select(n);
                    n->exec(g);
                    g.dispatch_tree_event();
                    g.dispatch_info_event();

                    if (mode == Gmode::KIFU) {
                        if (n->is_protected()) {
                            set_auto_increment(-1);
                        } else if (!is_auto_increment()) {
                            set_auto_increment(g.board.get_kifu().size() - 1);
                        }
                    }
                    return true;
                }
            }
        }

        return do_put_stone(cell, x, y, true);
    }

    case Gmode::SOLVE:
        return do_make_solve_move(cell, x, y);
    }

    return false;
}

bool Game::do_set_markup(std::string id, int x, int y, std::string label)
{
    if (mode != Gmode::CREATE && mode != Gmode::ANSWER) {
        return false;
    }

    // 解答作成モードのときは、石を置いてからじゃないとマークをつけれない。
    if (mode == Gmode::ANSWER && !route.current()->is_move()) {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();
    cur->merge_property(id, x, y, label);

    g.dispatch_tree_event();

    return true;
}

static std::string ID_MA = "MA";
static std::string ID_TR = "TR";
static std::string ID_CR = "CR";

bool Game::set_mark(int cell, int x, int y)
{
    std::string id;

    if (cell == CELL_MA) {
        id = ID_MA;
    } else if (cell == CELL_TR) {
        id = ID_TR;
    } else if (cell == CELL_CR) {
        id = ID_CR;
    } else {
        return false;
    }

    return do_set_markup(id, x, y);
}

static std::string ID_LB = "LB";

bool Game::set_label(std::string s, int x, int y)
{
    if (x == 0) {
        return false;
    }

    if (s.length() > 3) {
        return false;
    }

    if (mode != Gmode::CREATE) {
        std::shared_ptr<Node> cur = route.current();
        cur->remove_property(PID::LB, x, y);
    }

    bool result = false;

    result = do_set_markup(ID_LB, x, y, s);

    g.dispatch_tree_event();

    return result;
}

static std::string ID_C = "C";

void Game::add_comment(std::string s)
{
    if (s == "") {
        return;
    }

    std::shared_ptr<Node> cur = route.current();
    cur->remove_property(PID::C);

    cur->properties.push_back(create_property(ID_C, s));

    g.dispatch_tree_event();

}

void Game::remove_comment()
{
    std::shared_ptr<Node> cur = route.current();
    cur->remove_property(PID::C);
    g.dispatch_tree_event();
}

void Game::save_node(std::ofstream& ofs, std::shared_ptr<Node> node, int depth)
{
    if (depth == 0) {
        ofs << *node << std::endl;
    } else if (depth == 1) {
        ofs << ";";

        for (auto& p : node->properties) {
            ofs << *p.get() << std::endl;
        }
    } else {
        ofs << *node;
    }

    if (node->children.empty()) {
        return;
    }

    if (node->children.size() == 1) {
        save_node(ofs, *node->children.begin(), depth+1);
        return;
    }

    for (auto child : node->children) {
        ofs << "(";
        save_node(ofs, child, depth+1);
        ofs << ")";
    }
}

std::string Game::get_filename()
{
    time_t t = time(nullptr);
    const tm* localTime = localtime(&t);
    std::stringstream s;

    s << "20" << localTime->tm_year - 100;
    s << "-" << localTime->tm_mon + 1;
    s << "-" << localTime->tm_mday;
    s << "_" << localTime->tm_hour;
    s << localTime->tm_min;
    s << localTime->tm_sec;
    s << ".sgf";

    return s.str();
}

bool comp(std::shared_ptr<Property>& p1, std::shared_ptr<Property>& p2)
{
    return p1->id() < p2->id();
}

bool Game::save()
{
    if (mode != Gmode::ANSWER) {
        return false;
    }

    std::string filename = get_filename();
    std::ofstream ofs(filename);

    (*root->children.begin())->properties.sort(comp);

    ofs << "(";
    save_node(ofs, root, 0);
    ofs << ")" << std::endl;

    std::cout << filename << std::endl;

    g.dispatch_tree_event();

    return true;
}

bool Game::undo()
{
    if (mode == Gmode::CREATE) {
        return false;
    }

    if (!route.can_undo()) {
        return false;
    }

    route.undo(g);

    std::shared_ptr<Node> cur = route.current();

    // 解答モードのとき自分の手も戻す
    if (mode == Gmode::SOLVE && route.can_undo()) {
        for (auto p : cur->properties) {
            bool found = false;

            if (my_stone == CELL_BLACK && p->pid() == PID::B) {
                found = true;
            } else if (my_stone == CELL_WHITE && p->pid() == PID::W) {
                found = true;
            }

            if (found) {
                route.undo(g);
                break;
            }
        }
    }

    g.dispatch_tree_event();
    g.dispatch_info_event();

    return true;
}

bool Game::redo_kifu()
{
    std::shared_ptr<Node> cur = route.current();

    if (cur->children.empty()) {
        return false;
    }

    std::shared_ptr<Node> next = *cur->children.begin();

    next->exec(g);
    route.select(next);
    g.dispatch_tree_event();
    g.dispatch_info_event();

    return true;
}

bool Game::redo()
{
    if (mode == Gmode::CREATE) {
        return false;
    }

    if (!route.can_redo()) {
        if (mode == Gmode::ANSWER || mode == Gmode::KIFU) {
            return redo_kifu();
        }

        return false;
    }

    route.redo(g);

    // 解答モードのとき相手の手もredoする。
    if (mode == Gmode::SOLVE && route.can_redo()) {
        std::shared_ptr<Node> next = route.next();

        if (next.use_count() != 0) {
            bool found = false;

            for (auto p : next->properties) {
                if (my_stone == CELL_BLACK && p->pid() == PID::W) {
                    found = true;
                } else if (my_stone == CELL_WHITE && p->pid() == PID::B) {
                    found = true;
                }

                if (found) {
                    route.redo(g);
                    break;
                }

            }
        }
    }

    g.dispatch_tree_event();
    g.dispatch_info_event();

    return true;
}

bool Game::change_to_answer_mode()
{
    if (mode != Gmode::CREATE) {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();

    set_auto_increment(0);

    route.remove_nodes_after_cur();
    cur->children.clear();

    route.set_min_undo();
    mode = Gmode::ANSWER;

    return true;
}

bool Game::change_to_free_mode()
{
    if (mode != Gmode::SOLVE && g.board.get_kifu().size() != 0) {
        return false;
    }

    mode = Gmode::FREE;

    return true;
}

bool Game::change_to_create_mode()
{
    if (mode != Gmode::ANSWER || route.has_min_undo_children()) {
        return false;
    }

    set_auto_increment(-1);

    route.clear_min_undo();
    mode = Gmode::CREATE;

    return true;
}

void Game::move_to_answer()
{
    while (route.can_undo()) {
        undo();
    }

    bool found = true;
    while (found) {
        found = false;
        std::shared_ptr<Node> cur = route.current();
        for (auto n : cur->children) {
            if (n->is_correct_path()) {
                n->exec(g);
                route.select(n);
                found = true;
            }
        }
    }

    g.dispatch_tree_event();
    g.dispatch_info_event();
}

std::list<Label> Game::get_numbers()
{
    std::list<Label> labels;

    const std::vector<Move>& kifu = g.board.get_kifu();
    std::map<Point, bool> check_exists;

    for (int i = kifu.size()-incre_start-1; i >= 0; i--) {
        int idx = incre_start + i;
        if (idx < 0 && idx >= (int)kifu.size()) {
            continue;
        }
        Move m = kifu[idx];
        Point pt = Point(m.x, m.y);

        // pass
        if (m.x == 20 && m.y == 20) {
            continue;
        }

        if (check_exists[pt]) {
            continue;
        }

        check_exists[pt] = true;
        labels.push_back(Label(std::to_string(i+1), pt.x, pt.y));
    }

    return labels;
}

static std::string ID_N = "N";

bool Game::toggle_correct()
{
    if (mode != Gmode::ANSWER) {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();

    if (!cur->has(PID::B) && !cur->has(PID::W)) {
        return false;
    }

    Point pt;

    for (auto itr = cur->properties.begin(); itr != cur->properties.end(); ) {
        PID pid = (*itr)->pid();

        if (pid == PID::CORRECT) {
            cur->remove_property(PID::N);
            (*itr)->undo(g);
            itr = cur->properties.erase(itr);
            g.dispatch_tree_event();
            return true;
        }

        if (pid == PID::B || pid == PID::W) {
            pt = (*itr)->point();
        }

        itr++;
    }

    cur->properties.push_back(create_property(ID_N, "correct"));
    auto p = create_property(ID_CORRECT, pt.x, pt.y);
    p->exec(g);
    cur->properties.push_back(p);
    g.dispatch_tree_event();
    return true;
}

bool Game::delete_node()
{
    if (mode != Gmode::ANSWER && mode != Gmode::KIFU) {
        return false;
    }

    std::shared_ptr<Node> cur = route.current();
    if (cur->is_protected()) {
        return false;
    }

    if (!cur->has(PID::B) && !cur->has(PID::W)) {
        if (cur->children.empty()) {
            return false;
        }

        cur->children.clear();
    } else {
        route.visit_parent();
        std::shared_ptr<Node> parent = route.current();

        bool result = parent->remove_child(cur);
        if (result == false) {
            return false;
        }

        cur->undo(g);
    }

    route.remove_nodes_after_cur();

    g.dispatch_tree_event();
    g.dispatch_info_event();

    if (mode == Gmode::KIFU && is_auto_increment() && incre_start == (int)g.board.get_kifu().size()) {
        set_auto_increment(-1);
    }

    return true;
}

std::vector<Move> Game::get_next()
{
    std::vector<Move> next;

    std::shared_ptr<Node> cur = route.current();

    for (auto n : cur->children) {
        Move m = n->get_move();

        next.push_back(m);
    }

    return next;
}

void Game::update_markups()
{
    for (int x = 1; x <= size; x++) {
        for (int y = 1; y <= size; y++) {
            g.board.clear_flag(x, y);
        }
    }

    for (auto p : route.current()->properties) {
        int cell = CELL_SPACE;
        PID pid = p->pid();

        if (pid == PID::MA) {
            cell = CELL_MA;
        } else if (pid == PID::TR) {
            cell = CELL_TR;
        } else if (pid == PID::CR) {
            cell = CELL_CR;
        } else if (pid == PID::CORRECT) {
            cell = CELL_CORRECT;
        } else if (pid == PID::WRONG) {
            cell = CELL_WRONG;
        } else if (pid == PID::LB) {
            for (auto val : p->vals()) {
                if (val.length() <= 3) {
                    continue;
                }

                Point pt = SGFPoint::val_to_point(val);
                cell = str_to_int(val.substr(3));

                g.board.set_flag(cell, pt.x, pt.y);
            }

            continue;
        }

        if (cell != CELL_SPACE) {
            for (auto pt : p->points()) {
                g.board.set_flag(cell, pt.x, pt.y);
            }
        }
    }
}

Point Game::transform(Point pt)
{
    if (mirror_n % 2 == 1) {
        pt.x = size - pt.x + 1;
    }

    switch (rotate_n % 4) {
    case 0:  // 無回転
        break;
    case 1:  // 90度回転
        pt = Point(size - pt.y + 1, pt.x);
        break;
    case 2:  // 180度回転
        pt = Point(size - pt.x + 1, size - pt.y + 1);
        break;
    default:  // 270度回転
        pt = Point(pt.y, size - pt.x + 1);
        break;
    }

    return pt;
}

Point Game::rev_trans(Point pt)
{
    switch (rotate_n % 4) {
    case 0:  // 無回転
        break;
    case 1:  // 270度回転
        pt = Point(pt.y, size - pt.x + 1);
        break;
    case 2:  // 180度回転
        pt = Point(size - pt.x + 1, size - pt.y + 1);
        break;
    default:  // 90度回転
        pt = Point(size - pt.y + 1, pt.x);
        break;
    }

    if (mirror_n % 2 == 1) {
        pt.x = size - pt.x + 1;
    }

    return pt;
}

// 黒を白と言いくるめる
int Game::flip(int stone)
{
    if (flip_n % 2 == 0) {
        return stone;
    }

    if (stone & CELL_BLACK) {
        stone &= ~CELL_BLACK;
        stone |= CELL_WHITE;
    } else if (stone & CELL_WHITE) {
        stone &= ~CELL_WHITE;
        stone |= CELL_BLACK;
    }

    return stone;
}

void Game::set_rand_trans()
{
    mirror_n = rand() % 2;
    rotate_n = rand() % 4;
    flip_n = rand() % 2;
}

G::G()
    : root(new Node(true)), board(DEFAULT_BOARD_SIZE)
{
    root->properties.push_back(create_property(ID_N, "root"));
    new_game(DEFAULT_BOARD_SIZE);
}

void G::new_game(int size)
{
    std::shared_ptr<Node> node(new Node(true));
    node->properties.push_back(create_property("FF", "4"));
    node->properties.push_back(create_property("GM", "1"));
    node->properties.push_back(create_property("SZ", std::to_string(size)));
    node->properties.push_back(create_property("PB", "Black"));
    node->properties.push_back(create_property("PW", "White"));
    root->children.push_back(node);

    std::unique_ptr<Game> game(new Game(*this, Gmode::CREATE, node, size));
    game->player_black = "Black";
    game->player_white = "White";
    games.push_back(std::move(game));
    games_i = games.size() - 1;

    node->exec(*this);

    std::shared_ptr<Node> empty(new Node(true));
    node->children.push_back(empty);

    current().get_route().select(empty);

    update_game();
}

bool G::delete_game()
{
    if (games.size() <= 1) {
        return false;
    }

    auto itr = games.begin() + games_i;
    games.erase(itr);

    if (games_i >= (int)games.size()) {
        games_i = games.size() - 1;
    }

    update_game();

    return true;
}

Game& G::current()
{
    return *games[games_i];
}

std::list<Game*> G::get_games()
{
    std::list<Game*> lst;

    for (auto& g : games) {
        lst.push_back(g.get());
    }

    return lst;
}

bool G::load(Gmode mode, const std::string& filename, bool is_append)
{
    std::ifstream ifs(filename, std::ios::in);

    if (!ifs) {
        std::cerr << "Error: couldn't open file: " << filename << std::endl;
        return false;
    }

    std::vector<std::shared_ptr<Node>> bk_games;
    for (auto n : root->children) {
        bk_games.push_back(n);
    }
    int n_children = root->children.size();
    if (!is_append) {
        root->children.clear();
    }

    int num = -1;
    Route route = Route(root);

    do {
        num++;

        Token t = get_token(ifs);

        if (t != Token::L_PAREN) {
            if (num == 0) {
                std::cerr << "Error: not found '('. file = " << filename << std::endl;
                root->children.clear();
                for (auto n : bk_games) {
                    root->children.push_back(n);
                }
                return false;
            }
            break;
        }
    } while (load_node(route, ifs));

    auto itr = root->children.begin();

    if (is_append) {
        for (int i = 0; i < n_children; i++) {
            itr++;
        }
    } else {
        games.clear();
    }

    for (; itr != root->children.end(); itr++) {
        auto n = *itr;

        int size = 13;
        for (auto p : n->properties) {
            if (p->pid() == PID::SZ) {
                p->int_val(&size);
                break;
            }
        }

        std::unique_ptr<Game> game(new Game(*this, mode, n, size));
        if (mode == Gmode::SOLVE) {
            game->set_rand_trans();
            game->set_auto_increment(0);
        }
        games.push_back(std::move(game));
        games_i = games.size() - 1;
        current().setup();
    }

    if (mode == Gmode::SOLVE) {
        shuffle();
    }

    games_i = 0;
    if (games.size() > 1) {
        current().get_route().redo_history(*this);
    }

    dispatch_pos_event();
    dispatch_tree_event();

    return true;
}

void G::shuffle()
{
    std::random_device rd;
    std::mt19937 generator(rd());
    std::shuffle(games.begin(), games.end(), generator);
}

std::string G::get_pos()
{
    std::string s = std::to_string(games_i + 1);
    s += " / ";
    s += std::to_string(games.size());

    return s;
}

void G::dispatch_pos_event()
{
    std::string s = get_pos();

    for (auto lsn : m_listeners) {
        lsn->on_pos(s);
    }
}

std::string G::get_info()
{
    if (games.size() == 0) {
        return "";
    }

    std::string s = "第 ";
    s += std::to_string(board.n_moves);
    s += " 手    ";
    s += "黒: ";
    s += current().player_black;
    s += " アゲハマ ";
    s += std::to_string(board.n_black_hama);
    s += "    ";
    s += "白: ";
    s += current().player_white;
    s += " アゲハマ ";
    s += std::to_string(board.n_white_hama);

    return s;
}

void G::dispatch_info_event()
{
    std::string s = get_info();

    for (auto lsn : m_listeners) {
        lsn->on_info(s);
    }
}

void G::dispatch_tree_event()
{
    if (games.empty()) {
        return;
    }

    Game& game = current();

    if (game.get_mode() == Gmode::SOLVE) {
        return;
    }

    for (auto lsn : m_listeners) {
        lsn->on_tree(current());
    }
}

void G::dispatch_comment_event()
{
    std::string s = get_comment();

    for (auto lsn : m_listeners) {
        lsn->on_comment(s);
    }
}

bool G::prev_game()
{
    if (games_i == 0) {
        return false;
    }

    games_i--;

    update_game();

    return true;
}

bool G::next_game()
{
    if (games_i+1 > (int)games.size()-1) {
        return false;
    }

    games_i++;

    update_game();

    return true;
}

// ゲームを切り替えたときに呼ぶ。
void G::update_game()
{
    current().get_route().redo_history(*this);

    dispatch_tree_event();
    dispatch_pos_event();
    dispatch_info_event();
}

std::string G::set_comment(std::string comment) {
    std::string old = current().set_comment(comment);

    dispatch_comment_event();

    return old;
}

void G::add_listener(GEventListener* listener)
{
    m_listeners.push_back(listener);
}

void G::on_exec(Property& prop)
{
    if (games.empty()) {
        return;
    }

    Game& game = current();

    if (prop.pid() == PID::WRONG && !game.is_wrong) {
        game.is_wrong = true;

        for (auto lsn : m_listeners) {
            lsn->on_wrong(game.get_sgf());
        }
    }
}
