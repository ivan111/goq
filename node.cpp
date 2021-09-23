#include "node.h"
#include "board.h"
#include "command.h"
#include "g.h"

SGFPoint::SGFPoint(int x, int y)
{
    m_point.x = x;
    m_point.y = y;

    m_is_valid = is_valid_point(m_point);

    if (m_is_valid) {
        m_val = point_to_val(x, y);
    }
}

SGFPoint::SGFPoint(int x1, int y1, int x2, int y2)
{
    m_is_compressed = true;

    m_point.x = x1;
    m_point.y = y1;
    m_second.x = x2;
    m_second.y = y2;

    m_is_valid = is_valid_compressed();

    if (m_is_valid) {
        m_val = point_to_val(x1, x1);
        m_val += ":";
        m_val += point_to_val(x2, x2);
    }
}

SGFPoint::SGFPoint(std::string val)
{
    m_val = val;

    if (val.length() == 2) {
        m_point = val_to_point(val);

        m_is_valid = is_valid_point(m_point);
    } else if (val.length() == 5) {
        if (val[2] != ':') {
            m_is_valid = false;
            return;
        }

        m_is_compressed = true;

        m_point = val_to_point(val.substr(0, 2));
        m_second = val_to_point(val.substr(3, 5));

        m_is_valid = is_valid_compressed();
    }
}

bool SGFPoint::is_valid_point(Point pt)
{
    if (pt.x == 20 && pt.y == 20) {
        return true;
    }

    if (pt.x < 1 || pt.x > 19 || pt.y < 1 || pt.y > 19) {
        return false;
    }

    return true;
}

bool SGFPoint::is_valid_compressed()
{
    bool v = false;

    v = is_valid_point(m_point);
    v &= is_valid_point(m_second);

    if (!v) {
        return v;
    }

    if (m_point.x == m_second.x && m_point.y == m_second.y) {
        return false;
    }

    if (m_point.x > m_second.x || m_point.y > m_second.y) {
        return false;
    }

    return true;
}

Point SGFPoint::point()
{
    return m_point;
}

int SGFPoint::x()
{
    return m_point.x;
}

int SGFPoint::y()
{
    return m_point.y;
}

int SGFPoint::x2()
{
    return m_second.x;
}

int SGFPoint::y2()
{
    return m_second.y;
}

std::string SGFPoint::val()
{
    return m_val;
}

Point SGFPoint::first()
{
    return m_point;
}

Point SGFPoint::second()
{
    return m_second;
}

bool SGFPoint::operator==(const SGFPoint &obj) const
{
    if (!m_is_valid || !obj.m_is_valid) {
        return false;
    }

    return m_val == obj.m_val;
}

std::string SGFPoint::point_to_val(int x, int y)
{
    std::string s;
    s += 'a' + (x-1);
    s += 'a' + (y-1);
    return s;
}

Point SGFPoint::val_to_point(std::string s)
{
    if (s.length() < 2) {
        return Point(0, 0);
    }

    char c1 = s[0];
    char c2 = s[1];
    int x = c1 - 'a' + 1;
    int y = c2 - 'a' + 1;

    return Point(x, y);
}

int str_to_int(std::string s)
{
    int v;

    if (s.length() == 0) {
        return 0;
    }

    if (s.length() == 1) {
        v = s[0];
    } else if (s.length() == 2) {
        v = s[0] << 8 | s[1];
    } else {
        v = s[0] << 16 | s[1] << 8 | s[0];
    }

    return v << LBL_SHIFT;
}

void Node::set_next_move()
{
    next_move.clear();

    for (auto child : children) {
        for (auto p : child->properties) {
            Point pt = p->point();
            if (p->pid() == PID::B && pt.x != 0) {
                Move m = Move(CELL_BLACK, pt.x, pt.y);
                next_move[m] = child;
                break;
            } else if (p->pid() == PID::W) {
                Move m = Move(CELL_WHITE, pt.x, pt.y);
                next_move[m] = child;
                break;
            }

        }
    }
}

std::map<Move, std::shared_ptr<Node>> Node::get_next_move()
{
    return next_move;
}

bool Node::exec(G& g)
{
    bool result = false;

    for (auto& prop : properties) {
        result |= prop->exec(g);
    }

    return result;
}

void Node::undo(G& g)
{
    auto rbegin = properties.rbegin();;
    auto rend = properties.rend();

    for (auto itr = rbegin; itr != rend; itr++) {
        (*itr)->undo(g);
    }
}

void Node::redo(G& g)
{
    for (auto& prop : properties) {
        prop->redo(g);
    }
}

bool Node::has(PID pid)
{
    for (auto p : properties) {
        if (p->pid() == pid) {
            return true;
        }
    }

    return false;
}

std::string Node::to_sgf()
{
    std::string s = ";";

    for (auto& prop : properties) {
        s += prop->to_sgf();
    }

    for (auto& child : children) {
        if (children.size() > 1) {
            s += '(';
            s += child->to_sgf();
            s += ')';
        } else {
            s += child->to_sgf();
        }
    }

    return s;
}

std::string Node::to_string(Game& g)
{
    std::string s;

    for (auto& prop : properties) {
        s += prop->to_string(g);
        s += " ";
    }

    return s;
}

std::ostream& operator<<(std::ostream& os, const Node& n)
{
    os << ";";

    for (auto& prop : n.properties) {
        os << *prop.get();
    }

    return os;
}

void traverse(std::shared_ptr<Node> node, std::list<std::shared_ptr<Node>>& route, node_func_ptr fn)
{
    route.push_back(node);

    fn(node, route);

    for (auto n : node->children) {
        traverse(n, route, fn);
    }

    route.pop_back();
}

void node_to_list(std::list<std::shared_ptr<Node>>& lst, std::shared_ptr<Node> node)
{
    lst.push_back(node);

    for (auto v : node->children) {
        node_to_list(lst, v);
    }
}

void node_to_list_main_path(std::list<std::shared_ptr<Node>>& lst, std::shared_ptr<Node> node)
{
    lst.push_back(node);

    if (node->children.empty()) {
        return;
    }

    auto first = node->children.begin();
    node_to_list(lst, *first);
}

bool Node::is_move()
{
    for (auto p : properties) {
        if (p->is_move()) {
            return true;
        }
    }

    return false;
}

bool Node::is_setup()
{
    for (auto p : properties) {
        if (p->is_setup()) {
            return true;
        }
    }

    return false;
}

bool Node::is_skip()
{
    for (auto p : properties) {
        if (!p->is_skip()) {
            return false;
        }
    }

    return true;
}

bool Property::is_move()
{
    return m_pid == PID::B || m_pid == PID::W;
}

bool Property::is_setup()
{
    return m_pid == PID::AB || m_pid == PID::AW || m_pid == PID::AE;
}

bool Property::is_skip()
{
    return m_cmd == nullptr;
}

int nop(int a, int b)
{
    (void) a;
    return b;
}

int toggle_flag(int a, int b)
{
    return a ^ b;
}

int set_flag(int a, int b)
{
    return a | b;
}

int set_label(int a, int b)
{
    b &= (LBL_MASK >> LBL_SHIFT);
    a &= ~LBL_MASK;
    return a | (b << LBL_SHIFT);
}

Property::Property(std::string id, std::list<std::string> vals)
    : m_id(id), m_vals(vals)
{
    if (id == "B" || id == "W") {
        int cell;

        if (id == "W") {
            cell = CELL_WHITE;
            m_pid = PID::W;
        } else {
            cell = CELL_BLACK;
            m_pid = PID::B;
        }

        std::string val;

        if (vals.empty()) {
            // pass
            val = "tt";
            vals.push_back(val);
        } else {
            val = *vals.begin();
        }

        SGFPoint pt = SGFPoint(val);

        if (pt.is_valid()) {
            m_points.push_back(pt);
            m_cmd = new MakeMoveCmd(cell, pt.x(), pt.y());
        }
        return;
    }

    if (id == "AB" || id == "AW" || id == "AE" || id == "MA" || id == "TR" || id == "CR" || id == "CORRECT" || id == "WRONG") {
        int cell;
        BinOpPtr fn = nullptr;

        if (id == "AB") {
            m_pid = PID::AB;
            cell = CELL_BLACK;
            fn = nop;
        } else if (id == "AW") {
            m_pid = PID::AW;
            cell = CELL_WHITE;
            fn = nop;
        } else if (id == "AE") {
            m_pid = PID::AE;
            cell = CELL_SPACE;
            fn = nop;
        } else if (id == "MA") {
            m_pid = PID::MA;
            cell = CELL_MA;
        } else if (id == "TR") {
            m_pid = PID::TR;
            cell = CELL_TR;
        } else if (id == "CR") {
            m_pid = PID::CR;
            cell = CELL_CR;
        } else if (id == "WRONG") {
            m_pid = PID::WRONG;
            cell = CELL_WRONG;
        } else if (id == "CORRECT") {
            m_pid = PID::CORRECT;
            cell = CELL_CORRECT;
        } else {
            return;
        }

        std::list<Move> moves;

        for (auto val : vals) {
            SGFPoint pt = SGFPoint(val);

            if (!pt.is_valid()) {
                continue;
            }

            m_points.push_back(pt);

            if (pt.is_compressed()) {
                for (int x = pt.x(); x <= pt.x2(); x++) {
                    for (int y = pt.y(); y <= pt.y2(); y++) {
                        moves.push_back(Move(cell, x, y));
                    }
                }
            } else {
                moves.push_back(Move(cell, pt.x(), pt.y()));
            }
        }

        if (moves.empty()) {
            return;
        }

        if (fn) {
            m_cmd = new BinOpCmd(fn, moves);
        }
        return;
    }

    if (id == "LB") {
        m_pid = PID::LB;

        if (vals.empty()) {
            return;
        }

        std::string val = *vals.begin();

        if (val.length() < 3) {
            return;
        }

        SGFPoint pt = SGFPoint(val.substr(0, 2));
        int v = str_to_int(val.substr(3));

        if (!pt.is_valid()) {
            return;
        }

        m_points.push_back(pt);
        std::list<Move> moves;
        moves.push_back(Move(v, pt.x(), pt.y()));

        m_cmd = new BinOpCmd(set_label, moves);
        return;
    }

    if (id == "C") {
        m_pid = PID::C;

        if (vals.empty()) {
            return;
        }

        m_cmd = new CommentCmd(*vals.begin());
        return;
    }

    if (id == "PL") {
        m_pid = PID::PL;
        return;
    }

    if (id == "PB") {
        m_pid = PID::PB;
        return;
    }

    if (id == "PW") {
        m_pid = PID::PW;
        return;
    }

    if (id == "N") {
        m_pid = PID::N;

        if (vals.empty()) {
            return;
        }

        std::string val = *vals.begin();

        if (val == "correct" || val == "Correct" || val == "CORRECT") {
            m_is_correct = true;
        }

        return;
    }

    if (id == "SZ") {
        m_pid = PID::SZ;

        if (vals.empty()) {
            return;
        }

        int size;

        if (!int_val(&size)) {
            return;
        }

        m_cmd = new SizeCmd(size);
        return;
    }
}

Property::~Property()
{
    if (m_cmd) {
        delete m_cmd;
    }
}

bool Property::exec(G& g)
{
    g.on_exec(*this);

    if (m_cmd) {
        return m_cmd->exec(g);
    }

    return false;
}

void Property::undo(G& g)
{
    if (m_cmd) {
        m_cmd->undo(g);
    }
}

void Property::redo(G& g)
{
    if (m_cmd) {
        m_cmd->redo(g);
    }
}

std::string Property::to_sgf()
{
    std::string s;

    if (m_pid == PID::CORRECT || m_pid == PID::WRONG) {
        return s;
    }

    s += m_id;

    for (auto val : m_vals) {
        s += '[';
        s += val;
        s += ']';
    }

    return s;
}

std::string Property::to_string(Game& g)
{
    std::string s;

    if (m_pid == PID::CORRECT || m_pid == PID::WRONG) {
        return s;
    }

    if (g.flip_n % 2 == 0) {
        s += m_id;
    } else {
        if (m_pid == PID::B) {
            s += "W";
        } else if (m_pid == PID::W) {
            s += "B";
        } else if (m_pid == PID::AB) {
            s += "AW";
        } else if (m_pid == PID::AW) {
            s += "AB";
        } else {
            s += m_id;
        }
    }

    if (m_pid == PID::B || m_pid == PID::W) {
        Point pt = point();

        if (pt.x == 20 && pt.y == 20) {
            s += "(pass)";
        } else {
            pt = g.transform(pt);
            s += '(';
            s += std::to_string(pt.x);
            s += ',';
            s += std::to_string(pt.y);
            s += ')';
        }
    } else if (m_pid == PID::AB || m_pid == PID::AW || m_pid == PID::AE) {
        for (auto p : m_points) {
            if (p.is_compressed()) {
                Point p1 = g.transform(p.first());
                Point p2 = g.transform(p.second());

                s += '(';
                s += std::to_string(p1.x);
                s += ',';
                s += std::to_string(p1.y);
                s += ':';
                s += std::to_string(p2.x);
                s += ',';
                s += std::to_string(p2.y);
                s += ')';
            } else {
                Point pt = g.transform(p.first());

                s += '(';
                s += std::to_string(pt.x);
                s += ',';
                s += std::to_string(pt.y);
                s += ')';
            }
        }
    } else {
        for (auto val : m_vals) {
            s += '[';
            s += val;
            s += ']';
        }
    }

    return s;
}

std::ostream& operator<<(std::ostream& os, const Property& p)
{
    if (p.m_pid == PID::CORRECT || p.m_pid == PID::WRONG) {
        return os;
    }

    os << p.m_id;

    for (std::string v : p.m_vals) {
        os << "[" << v << "]";
    }

    return os;
}

bool Property::int_val(int* v)
{
    if (m_vals.empty()) {
        return false;
    }

    try {
        auto s = *m_vals.begin()++;
        *v = stoi(s);
    } catch (std::invalid_argument& e) {
        return false;
    } catch (std::out_of_range& e) {
        return false;
    }

    return true;
}

std::string Property::val()
{
    if (m_vals.empty()) {
        return "";
    }

    return *m_vals.begin();
}

Point Property::point()
{
    Point pt = Point(0, 0);

    if (m_points.empty()) {
        return pt;
    }

    SGFPoint spt = *m_points.begin();
    return Point(spt.x(), spt.y());
}

std::list<Point> Property::points()
{
    std::list<Point> lst;

    for (auto p : m_points) {
        if (!p.is_valid()) {
            continue;
        }

        if (p.is_compressed()) {
            for (int x = p.x(); x <= p.x2(); x++) {
                for (int y = p.y(); y <= p.y2(); y++) {
                    lst.push_back(Point(x, y));
                }
            }
        } else {
            lst.push_back(p.point());
        }
    }

    return lst;
}

bool Node::remove_child(std::shared_ptr<Node> child)
{
    auto begin = children.begin();
    auto end = children.end();

    for (auto itr = begin; itr != end; itr++) {
        if (*itr == child && !(*itr)->is_protected()) {
            children.erase(itr);
            return true;
        }
    }

    return false;
}

bool Node::remove_property(PID pid)
{
    auto begin = properties.begin();
    auto end = properties.end();

    for (auto itr = begin; itr != end; itr++) {
        if ((*itr)->pid() == pid) {
            properties.erase(itr);
            return true;
        }
    }

    return false;
}

bool Node::remove_property(PID pid, int x, int y)
{
    auto begin = properties.begin();
    auto end = properties.end();

    for (auto itr = begin; itr != end; itr++) {
        Point pt = (*itr)->point();
        if ((*itr)->pid() == pid && pt.x == x && pt.y == y) {
            properties.erase(itr);
            return true;
        }
    }

    return false;
}

bool Node::remove_property_val(int x, int y)
{
    bool result = false;

    auto begin = properties.begin();
    auto end = properties.end();

    for (auto itr = begin; itr != end;) {
        std::shared_ptr<Property> p = *itr;

        bool incr = true;

        for (auto pt : p->points()) {
            if (pt.x == x && pt.y == y) {
                if (p->vals().size() == 1) {
                    itr = properties.erase(itr);
                    incr = false;
                } else {
                    p->remove_val(SGFPoint(pt.x, pt.y));
                }

                result = true;
                break;
            }
        }

        if (incr) {
            itr++;
        }
    }

    return result;
}

bool Property::remove_val(SGFPoint pt)
{
    bool result = false;

    auto begin = m_vals.begin();
    auto end = m_vals.end();

    for (auto itr = begin; itr != end; itr++) {
        if (*itr == pt.val()) {
            m_vals.erase(itr);
            result = true;
            break;
        }
    }

    if (result) {
        auto pt_begin = m_points.begin();
        auto pt_end = m_points.end();

        for (auto itr = pt_begin; itr != pt_end; itr++) {
            if (*itr == pt) {
                m_points.erase(itr);
                return true;
            }
        }
    }

    return result;
}

Move Node::get_move()
{
    Move m;

    for (auto p : properties) {
        Point pt = p->point();

        if (p->pid() == PID::B) {
            return Move(CELL_BLACK, pt.x, pt.y);
        } else if (p->pid() == PID::W) {
            return Move(CELL_WHITE, pt.x, pt.y);
        }
    }

    return m;
}

std::shared_ptr<Property> create_property(std::string id, std::string val)
{
    std::list<std::string> vals;
    vals.push_back(val);
    std::shared_ptr<Property> prop(new Property(id, vals));

    return prop;
}

std::shared_ptr<Property> create_property(std::string id, int x, int y)
{
    return create_property(id, SGFPoint::point_to_val(x, y));
}

std::shared_ptr<Node> create_node(std::string id, std::string val)
{
    std::shared_ptr<Property> prop = create_property(id, val);
    std::shared_ptr<Node> node(new Node());
    node->properties.push_back(prop);

    return node;
}

std::shared_ptr<Node> create_node(std::string id, int x, int y)
{
    return create_node(id, SGFPoint::point_to_val(x, y));
}

std::shared_ptr<Node> create_node(std::string id, std::string label, int x, int y)
{
    return create_node(id, SGFPoint::point_to_val(x, y) + ":" + label);
}

bool Property::merge(SGFPoint pt, std::string label)
{
    if (!pt.is_valid()) {
        return false;
    }

    std::string v = pt.val();

    if (label != "") {
        v += ":";
        v += label;
    }

    for (auto itr = m_vals.begin(); itr != m_vals.end(); itr++) {
        Point p = SGFPoint::val_to_point(*itr);

        if (p == pt.point()) {  // TODO: compressedの場合でも一致してしまう
            if (label == "") {
                return false;
            } else if (*itr == v) {
                return false;
            } else {  // ラベルの値が変わっているので書き換え
                *itr = v;
                return true;
            }
        }
    }

    if (label == "") {
        m_vals.push_back(pt.val());
    } else {
        m_vals.push_back(pt.val() + ":" + label);
    }
    m_points.push_back(pt);

    return true;
}

bool Node::merge_property(std::string id, int x, int y, std::string label)
{
    for (auto p : properties) {
        if (p->id() == id) {
            SGFPoint pt = SGFPoint(x, y);
            return p->merge(pt, label);
        }
    }

    if (label == "") {
        properties.push_back(create_property(id, x, y));
    } else {
        properties.push_back(create_property(id, SGFPoint::point_to_val(x, y) + ":" + label));
    }
    return true;
}
