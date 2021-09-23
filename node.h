#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <string>
#include "board.h"

class Command;

#define MAX_PROP_ID_LEN (7)

class G;
class Game;

enum PID {
    UNKNOWN, SZ, PB, PW, C, PL, N, B, W, AB, AW, AE, LB, MA, TR, CR,
    CORRECT, WRONG,
};

class SGFPoint
{
    bool m_is_valid = false;
    bool m_is_compressed = false;
    Point m_point = Point(0, 0);
    Point m_second = Point(0, 0);
    std::string m_val = "";

    bool is_valid_point(Point pt);
    bool is_valid_compressed();
public:
    SGFPoint(int x, int y);
    SGFPoint(int x1, int y1, int x2, int y2);
    SGFPoint(std::string val);

    bool is_valid() { return m_is_valid; };

    Point point();
    int x();
    int y();
    int x2();
    int y2();
    std::string val();

    bool is_compressed() { return m_is_compressed; };
    Point first();
    Point second();

    bool operator==(const SGFPoint &obj) const;

    static std::string point_to_val(int x, int y);
    static Point val_to_point(std::string s);
};

class Property
{
    bool m_is_correct = false;
protected:
    std::string m_id;
    PID m_pid = PID::UNKNOWN;
    std::list<std::string> m_vals;
    std::list<SGFPoint> m_points;
    Command* m_cmd = nullptr;
public:
    Property(std::string id, std::list<std::string> vals);
    ~Property();
    bool exec(G& g);
    void undo(G& g);
    void redo(G& g);

    std::string to_string(Game& g);
    std::string to_sgf();
    friend std::ostream& operator<<(std::ostream& os, const Property& p);

    const std::string& id() { return m_id; };
    PID pid() { return m_pid; };

    bool int_val(int* v);
    std::string val();
    std::list<std::string> vals() { return m_vals; };
    Point point();
    std::list<Point> points();

    bool merge(SGFPoint pt, std::string label="");
    bool remove_val(SGFPoint);

    bool is_move();
    bool is_setup();
    bool is_skip();
    bool is_correct() { return m_is_correct; };
};

class Node
{
    bool m_is_protected = false;
    bool m_is_correct_path = false;
    std::map<Move, std::shared_ptr<Node>> next_move;
    void *tree_id = nullptr;
public:
    Node(bool is_protected=false)
        : m_is_protected(is_protected) {};
    std::list<std::shared_ptr<Property>> properties;
    std::list<std::shared_ptr<Node>> children;

    void set_tree_id(void* id) { tree_id = id; };

    bool exec(G& g);
    void undo(G& g);
    void redo(G& g);

    std::string to_string(Game& g);
    std::string to_sgf();
    friend std::ostream& operator<<(std::ostream& os, const Node& p);

    bool has(PID pid);

    Move get_move();

    void set_next_move();
    std::map<Move, std::shared_ptr<Node>> get_next_move();

    bool merge_property(std::string id, int x, int y, std::string label="");

    bool remove_child(std::shared_ptr<Node> child);
    bool remove_property(PID pid);
    bool remove_property(PID pid, int x, int y);
    bool remove_property_val(int x, int y);

    bool is_move();
    bool is_setup();
    bool is_skip();
    bool is_protected() { return m_is_protected; };
    bool is_correct_path() { return m_is_correct_path; };
    void set_correct_path(bool v) { m_is_correct_path = v; };
};

using node_func_ptr = void (*)(std::shared_ptr<Node> node, std::list<std::shared_ptr<Node>> route);

void traverse(std::shared_ptr<Node> node, std::list<std::shared_ptr<Node>>& route, node_func_ptr fn);

std::shared_ptr<Property> create_property(std::string id, std::string val);
std::shared_ptr<Property> create_property(std::string id, int x, int y);
std::shared_ptr<Node> create_node(std::string id, std::string val);
std::shared_ptr<Node> create_node(std::string id, int x, int y);
std::shared_ptr<Node> create_node(std::string id, std::string label, int x, int y);

typedef std::vector<std::shared_ptr<Node>> node_vec;
typedef std::list<std::shared_ptr<Node>>::iterator node_list_itr;

void node_to_list(std::list<std::shared_ptr<Node>>& lst, std::shared_ptr<Node> node);
void node_to_list_main_path(std::list<std::shared_ptr<Node>>& lst, std::shared_ptr<Node> node);

int str_to_int(std::string s);

#endif
