#ifndef G_H
#define G_H

#include <memory>
#include <string>
#include <vector>
#include "board.h"
#include "node.h"

class Node;
class Property;

enum class Gmode {
    CREATE,
    ANSWER,
    SOLVE,
    FREE,
    KIFU,
};

struct Label
{
    std::string label;
    int x, y;

    Label(std::string label, int x, int y)
        : label(label), x(x), y(y) {};
};

using ListenerPtr = void (*)(std::string);

class G;

// 辿った木を記録。履歴の管理に使用
class Route
{
    std::vector<std::shared_ptr<Node>> m_route;
    int m_idx; // 現在のrouteの位置を指す
    int m_min_idx = 0;  // これより前にはundoできない
public:
    Route(std::shared_ptr<Node> root);
    std::shared_ptr<Node> current();
    std::shared_ptr<Node> next();
    bool select(std::shared_ptr<Node> node);
    bool visit_parent();
    void visit_min_undo();
    bool has_min_undo_children();
    bool exists(std::shared_ptr<Node> node);
    void set_min_undo();
    void clear_min_undo();
    int remove_nodes_after_cur();

    void append(std::shared_ptr<Node> node, bool do_select=true);

    bool can_undo();
    bool undo(G& g);
    bool can_redo();
    bool redo(G& g);
    void redo_history(G& g);

    friend std::ostream& operator<<(std::ostream& os, const Route& obj);
};

class Game
{
    G& g;
    Gmode mode;
    std::shared_ptr<Node> root;
    Route route;
    int size;
    std::string m_comment = "";

    int my_stone = 0;
    int incre_start = -1;

    bool do_put_stone(int cell, int x, int y, bool is_move, bool is_wrong=false);
    bool place_stone(int cell, int x, int y);
    bool do_make_solve_move(int cell, int x, int y);
    bool do_set_markup(std::string id, int x, int y, std::string label="");
    std::shared_ptr<Node> search_next_node(int cell, int x, int y);

    void remove_no_use_history();
    bool set_my_stone();
    void set_correct_path();
    void set_wrong_mark();

    void save_node(std::ofstream& ofs, std::shared_ptr<Node> node, int depth);
    std::string get_filename();
public:
    Game(G& g, Gmode mode, std::shared_ptr<Node> root, int size);

    void setup();

    Gmode get_mode() { return mode; };
    std::shared_ptr<Node> get_root() { return root; };
    Route& get_route() { return route; };
    int get_my_stone() { return my_stone; };
    std::string get_sgf();
    std::string comment() { return m_comment; };
    std::string set_comment(std::string s) { m_comment = s; return m_comment; };

    void print_nodes();
    void print_route();

    bool put_stone(int cell, int x, int y);
    bool toggle_mark(int cell, int x, int y);
    bool set_mark(int cell, int x, int y);
    bool set_label(std::string s, int x, int y);
    bool remove_label(int x, int y);
    void add_comment(std::string s);
    void remove_comment();

    bool save();
    bool undo();
    bool redo();
    bool redo_kifu();

    bool change_to_answer_mode();
    bool change_to_free_mode();
    bool change_to_create_mode();

    void move_to_answer();

    void set_auto_increment(int i) { incre_start = i; };
    bool is_auto_increment() { return incre_start != -1; };
    std::list<Label> get_numbers();

    bool toggle_correct();
    bool delete_node();

    std::vector<Move> get_next();
    void update_markups();

    bool is_wrong = false;

    void set_rand_trans();

    Point transform(Point pt);
    Point rev_trans(Point pt);
    int flip(int stone);

    int mirror_n = 0;
    int rotate_n = 0;
    int flip_n = 0;

    std::string player_black = "";
    std::string player_white = "";
};

class GEventListener
{
public:
    virtual void on_comment(std::string comment) = 0;
    virtual void on_pos(std::string pos) = 0;
    virtual void on_wrong(std::string sgf) = 0;
    virtual void on_info(std::string info) = 0;
    virtual void on_tree(Game& g) = 0;
};

class G
{
    std::shared_ptr<Node> root;

    std::vector<std::unique_ptr<Game>> games;
    int games_i;

    std::vector<GEventListener*> m_listeners;
    void dispatch_pos_event();
    void update_game();
public:
    Board board;

    G();

    Game& current();
    std::list<Game*> get_games();

    std::shared_ptr<Node> get_root() { return root; };

    void new_game(int size);
    bool delete_game();

    bool load(Gmode mode, const std::string& filename, bool is_append=false);

    bool prev_game();
    bool next_game();

    void shuffle();

    std::string get_comment() { return current().comment(); };
    std::string set_comment(std::string comment);
    void add_listener(GEventListener* listener);
    void dispatch_info_event();
    void dispatch_tree_event();
    void dispatch_comment_event();
    std::string get_info();
    std::string get_pos();

    void on_exec(Property& prop);
};

#endif
