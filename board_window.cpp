#include <wx/graphics.h>
#include "board_window.h"
#include "frame.h"
#include "node.h"

#define POINT_W (3)

// 点（天元、星等）
static int points9[][2] = {{3, 3}, {3, 7}, {5, 5}, {7, 3}, {7, 7}};
static int points13[][2] = {{4, 4}, {4, 7}, {4, 10}, {7, 4}, {7, 7}, {7, 10}, {10, 4}, {10, 7}, {10, 10}};
static int points19[][2] = {{4, 4}, {4, 10}, {4, 16}, {10, 4}, {10, 10}, {10, 16}, {16, 4}, {16, 10}, {16, 16}};

static int points_len9 = sizeof(points9) / sizeof(int[2]);
static int points_len13 = sizeof(points13) / sizeof(int[2]);
static int points_len19 = sizeof(points19) / sizeof(int[2]);

// 色
static wxColour board_color(214, 130, 34);
static wxBrush board_brush(board_color);

static wxColour black_stone_colour(0x10, 0x10, 0x10);
static wxColour white_stone_colour(0xe0, 0xe0, 0xe0);
static wxBrush black_stone_brush(black_stone_colour);
static wxBrush white_stone_brush(white_stone_colour);
static wxBrush tr_black_stone_brush(wxColour(0x10, 0x10, 0x10, 0x80));
static wxBrush tr_white_stone_brush(wxColour(0xe0, 0xe0, 0xe0, 0x80));
static wxPen black_stone_pen(black_stone_colour, 3);
static wxPen white_stone_pen(white_stone_colour, 3);

static wxPen black_pen(*wxBLACK, 3);
static wxPen white_pen(*wxWHITE, 3);
static wxPen blue_pen(*wxBLUE, 5);
static wxPen red_pen(*wxRED, 5);

// 謎の数字
static const double fzemgaqojefaslkjhfa = 0.866;

BoardWindow::BoardWindow(MyFrame* frame, G& g)
    : wxWindow(frame, wxID_ANY), g(g), frame(frame)
{
    SetMinSize(wxSize(MIN_WIDTH, MIN_WIDTH));

    Bind(wxEVT_PAINT, &BoardWindow::OnPaint, this, wxID_ANY);
    Bind(wxEVT_SIZE, &BoardWindow::OnSize, this, wxID_ANY);

    // key
    Bind(wxEVT_KEY_DOWN, &BoardWindow::OnKeyDown, this, wxID_ANY);

    // mouse
    Bind(wxEVT_MOTION, &BoardWindow::OnMove, this, wxID_ANY);
    Bind(wxEVT_ENTER_WINDOW, &BoardWindow::OnEnter, this, wxID_ANY);
    Bind(wxEVT_LEAVE_WINDOW, &BoardWindow::OnLeave, this, wxID_ANY);
    Bind(wxEVT_LEFT_DOWN, &BoardWindow::OnClick, this, wxID_ANY);
    Bind(wxEVT_RIGHT_DOWN, &BoardWindow::OnClick, this, wxID_ANY);
    Bind(wxEVT_MOUSEWHEEL, &BoardWindow::OnWheel, this, wxID_ANY);
}

void DrawLabel(wxPaintDC& dc, wxFont* fonts[3], int stone, std::string s, int x, int y)
{
    if (stone == CELL_SPACE) {
        dc.SetBackgroundMode(wxSOLID);
        dc.SetTextForeground(*wxBLACK);
        dc.SetTextBackground(board_color);
    } else if (stone == CELL_BLACK) {
        dc.SetTextForeground(*wxWHITE);
    } else if (stone == CELL_WHITE) {
        dc.SetTextForeground(*wxBLACK);
    }

    if (s.length() == 1) {
        dc.SetFont(*fonts[0]);
    } else if (s.length() == 2) {
        dc.SetFont(*fonts[1]);
    } else {
        dc.SetFont(*fonts[2]);
    }

    wxCoord w, h;
    dc.GetTextExtent(s, &w, &h);
    dc.DrawText(s, x - w/2, y - h/2);
    dc.SetBackgroundMode(wxTRANSPARENT);
}

void DrawMA(wxPaintDC& dc, wxPen* pen, int cell, double st_x, double st_y, double space_w)
{
    int ma_d = space_w / 3.5;
    int sp = space_w / 2.5;

    dc.SetPen(wxNullPen);
    if ((cell & 3) == CELL_SPACE) {
        dc.SetBrush(board_brush);
    } else {
        dc.SetBrush(wxNullBrush);
    }
    dc.DrawRectangle(st_x - sp, st_y - sp, sp*2, sp*2);

    if (cell & CELL_MA) {
        dc.SetPen(*pen);
    } else {
        dc.SetPen(red_pen);
    }

    dc.DrawLine(st_x - ma_d, st_y - ma_d, st_x + ma_d, st_y + ma_d);
    dc.DrawLine(st_x + ma_d, st_y - ma_d, st_x - ma_d, st_y + ma_d);
}

void DrawTR(wxPaintDC& dc, wxPen* pen, int cell, double st_x, double st_y, double space_w)
{
    double tr_d1 = space_w / 2.9;
    double tr_d2 = tr_d1 * fzemgaqojefaslkjhfa;
    double tr_d3 = tr_d1 / 2;

    int sp = space_w / 2;

    dc.SetPen(wxNullPen);
    if ((cell & 3) == CELL_SPACE) {
        dc.SetBrush(board_brush);
    } else {
        dc.SetBrush(wxNullBrush);
    }
    dc.DrawRectangle(st_x - sp, st_y - sp, sp*2, sp*2);

    dc.SetPen(*pen);

    dc.DrawLine(st_x, st_y - tr_d1, st_x + tr_d2, st_y + tr_d3);
    dc.DrawLine(st_x + tr_d2, st_y + tr_d3, st_x - tr_d2, st_y + tr_d3);
    dc.DrawLine(st_x - tr_d2, st_y + tr_d3, st_x, st_y - tr_d1);
}

void DrawCR(wxPaintDC& dc, wxPen* pen, int cell, double st_x, double st_y, double space_w)
{
    double cr_r = space_w / 2.9;
    int sp = space_w / 2;

    dc.SetPen(wxNullPen);
    if ((cell & 3) == CELL_SPACE) {
        dc.SetBrush(board_brush);
    } else {
        dc.SetBrush(wxNullBrush);
    }
    dc.DrawRectangle(st_x - sp, st_y - sp, sp*2, sp*2);

    if (cell & CELL_CR) {
        dc.SetPen(*pen);
    } else {
        dc.SetPen(blue_pen);
    }
    dc.SetBrush(wxNullBrush);

    dc.DrawCircle(st_x, st_y, cr_r);
}

void DrawStr(wxPaintDC& dc, wxFont* font, std::string s)
{
    dc.SetTextForeground(*wxBLACK);
    dc.SetFont(*font);
    dc.DrawText(s, 0, 0);
}

void BoardWindow::OnPaint(wxPaintEvent& e)
{
    (void)e;

    Game& game = g.current();

    wxPaintDC dc(this);
    wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
    if (!gc) {
        return;
    }

    game.update_markups();

    // サイズ
    int board_size = g.board.get_size();
    double space_w = double(width) / (board_size + 1);
    double focus_w = space_w / 3;
    double pad = space_w;
    int lattice_w = width - pad * 2;

    // ボード背景
    dc.SetPen(wxNullPen);
    dc.SetBrush(board_brush);
    dc.DrawRectangle(0, 0, width, width);

    // 線
    dc.SetPen(black_pen);

    for (int i = 0; i < board_size; i++) {
        int a = pad;
        int b = pad + space_w * i;

        dc.DrawLine(a, b, a + lattice_w, b);
        dc.DrawLine(b, a, b, a + lattice_w);
    }

    // 点
    int (*points)[2] = NULL;
    dc.SetBrush(*wxBLACK_BRUSH);
    int points_len = 0;
    switch (board_size) {
    case 9:
        points = points9;
        points_len = points_len9;
        break;
    case 13:
        points = points13;
        points_len = points_len13;
        break;
    case 19:
        points = points19;
        points_len = points_len19;
        break;
    }

    if (points != NULL) {
        for (int i = 0; i < points_len; i++) {
            int pt_x = pad + space_w * (points[i][0]-1);
            int pt_y = pad + space_w * (points[i][1]-1);

            dc.DrawCircle(pt_x, pt_y, POINT_W);
        }
    }

    // 石等
    dc.SetPen(wxNullPen);

    Point cur = g.board.get_cur();
    Point last = game.transform(g.board.get_last());

    wxFont* fonts[3];

    fonts[0] = new wxFont(space_w / 2, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    fonts[1] = new wxFont(space_w / 2.5, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);
    fonts[2] = new wxFont(space_w / 3, wxFONTFAMILY_DEFAULT,
            wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL);

    for (int x = 1; x <= board_size; x++) {
        for (int y = 1; y <= board_size; y++) {
            Point pos = game.transform(Point(x, y));
            int stone = game.flip(g.board.get_val(x, y));
            int cell = game.flip(g.board.get_cell(x, y));

            if (cell == 0 && pos != cur) {
                continue;
            }

            double st_x = pad + space_w * (pos.x-1);
            double st_y = pad + space_w * (pos.y-1);
            double st_r = space_w / 2.1;

            wxPen* stone_pen = &black_stone_pen;

            if (stone == CELL_BLACK) {
                stone_pen = &white_stone_pen;
                dc.SetPen(wxNullPen);
                dc.SetBrush(black_stone_brush);
                dc.DrawCircle(st_x, st_y, st_r);
            } else if (stone == CELL_WHITE) {
                dc.SetPen(*wxBLACK_PEN);
                dc.SetBrush(white_stone_brush);
                dc.DrawCircle(st_x, st_y, st_r);
            }

            if (cell & CELL_MA || cell & CELL_WRONG) {  // X
                DrawMA(dc, stone_pen, cell, st_x, st_y, space_w);
            } else if (cell & CELL_TR) {  // triangle
                DrawTR(dc, stone_pen, cell, st_x, st_y, space_w);
            } else if (cell & CELL_CR || cell & CELL_CORRECT) {  // circle
                DrawCR(dc, stone_pen, cell, st_x, st_y, space_w);
            } else if (cell & LBL_MASK) {
                std::string s;
                int n = (cell & LBL_MASK) >> LBL_SHIFT;
                char lb1 = (n & 0xff0000) >> 16;
                char lb2 = (n & 0xff00) >> 8;
                char lb3 = n & 0xff;
                if (lb1 != 0) {
                    s += lb1;
                }
                if (lb2 != 0) {
                    s += lb2;
                }
                if (lb3 != 0) {
                    s += lb3;
                }

                DrawLabel(dc, fonts, stone, s, st_x, st_y);
            }

            // 最後に打った石
            if (pos == last && !game.is_auto_increment()) {
                dc.SetPen(wxNullPen);
                dc.SetBrush(*wxRED_BRUSH);

                double last_x = st_x - focus_w / 2;
                double last_y = st_y - focus_w / 2;

                dc.DrawRectangle(last_x, last_y, focus_w, focus_w);
            }

            // フォーカス
            if (pos == cur) {
                int my_stone = game.flip(game.get_my_stone());
                if (stone != CELL_SPACE || my_stone == CELL_SPACE) {
                    dc.SetPen(wxNullPen);
                    dc.SetBrush(*wxBLUE_BRUSH);

                    double fc_x = st_x - focus_w / 2;
                    double fc_y = st_y - focus_w / 2;

                    dc.DrawRectangle(fc_x, fc_y, focus_w, focus_w);
                } else {
                    if (my_stone == CELL_BLACK) {
                        gc->SetPen(wxNullPen);
                        gc->SetBrush(tr_black_stone_brush);
                    } else {
                        gc->SetPen(wxNullPen);
                        gc->SetBrush(tr_white_stone_brush);
                    }

                    gc->DrawEllipse(st_x - space_w/2, st_y - space_w/2, st_r*2, st_r*2);
                }
            }
        }
    }

    /*
    // 記号
    for (int i = 0; i < n_markups; i++) {
        PID pid  = markups[i];

        for (Point pt : game.markups(pid)) {
            Point pos = game.transform(Point(pt.x, pt.y));
            int stone = game.flip(g.board.get_val(pt.x, pt.y));

            double st_x = pad + space_w * (pos.x-1);
            double st_y = pad + space_w * (pos.y-1);

            wxPen* pen = &black_stone_pen;

            if (stone == CELL_BLACK) {
                pen = &white_stone_pen;
            }

            if (pid == PID::MA || pid  == PID::WRONG) {
                DrawMA(dc, pen, stone, pid, st_x, st_y, space_w);
            } else if (pid == PID::TR) {
                DrawTR(dc, pen, stone, st_x, st_y, space_w);
            } else if (pid == PID::CR || pid == PID::CORRECT) {
                DrawCR(dc, pen, stone, pid, st_x, st_y, space_w);
            }
        }
    }
    */

    // 自動インクリメント数字の表示
    if (game.is_auto_increment()) {
        for (auto label : game.get_numbers()) {
            if (!g.board.has_stone(label.x, label.y)) {
                continue;
            }

            int cell = game.flip(g.board.get_cell(label.x, label.y));

            if (cell & LBL_MASK) {
                continue;
            }

            int stone = game.flip(g.board.get_val(label.x, label.y));
            Point pt = game.transform(Point(label.x, label.y));
            double st_x = pad + space_w * (pt.x-1);
            double st_y = pad + space_w * (pt.y-1);

            DrawLabel(dc, fonts, stone, label.label, st_x, st_y);
        }
    }

    // 変化
    Gmode mode = game.get_mode();
    if (mode == Gmode::ANSWER || mode == Gmode::KIFU) {
        std::vector<struct Move> next = game.get_next();

        if (mode == Gmode::KIFU && next.size() == 1) {
            // 何もしない
        } else {
            for (int i = 0; i < (int) next.size(); i++) {
                struct Move m = next[i];

                if (m.x == 0) {
                    continue;
                }

                std::string s(1, 'A' + i);

                int stone = game.flip(g.board.get_val(m.x, m.y));
                double st_x = pad + space_w * (m.x-1);
                double st_y = pad + space_w * (m.y-1);

                DrawLabel(dc, fonts, stone, s, st_x, st_y);
            }
        }
    }


    // pass
    if (g.board.is_pass) {
        if (game.flip(g.board.pass_stone) == CELL_BLACK) {
            DrawStr(dc, fonts[0], "Black: pass");
        } else if (game.flip(g.board.pass_stone) == CELL_WHITE) {
            DrawStr(dc, fonts[0], "White: pass");
        } else {
            DrawStr(dc, fonts[0], "bug");
        }
    }

    delete gc;
}

void BoardWindow::OnSize(wxSizeEvent& e)
{
    (void) e;

    wxSize size = GetSize();
    int w = size.GetWidth();
    int h = size.GetHeight();
    width = std::min(w, h);

    Refresh();
}

void BoardWindow::OnMove(wxMouseEvent& e)
{
    int board_size = g.board.get_size();
    Point prev = g.current().transform(g.board.get_cur());

    double space_w = double(width) / (board_size + 1);
    double pad = space_w;
    double start = pad - space_w / 2;

    int x = (e.GetX() - start) / space_w + 1;
    int y = (e.GetY() - start) / space_w + 1;

    g.board.set_cur(x, y);

    Point cur = g.current().transform(g.board.get_cur());

    if (prev != cur) {
        Refresh();
    }
}

void BoardWindow::OnEnter(wxMouseEvent& e)
{
    (void) e;
    SetFocus();
}

void BoardWindow::OnLeave(wxMouseEvent& e)
{
    (void) e;

    Point cur = g.board.get_cur();

    if (cur) {
        g.board.set_cur(0, 0);
        Refresh();
    }
}

void BoardWindow::OnClick(wxMouseEvent& e)
{
    Point cur = g.current().rev_trans(g.board.get_cur());

    SetFocus();

    if (cur) {
        if (g.board.has_stone(cur)) {
            return;
        }

        int stone;

        if (e.LeftDown()) {
            stone = g.current().flip(CELL_BLACK);
        } else if (e.RightDown()) {
            stone = g.current().flip(CELL_WHITE);
        } else {
            return;
        }

        if (g.current().put_stone(stone, cur.x, cur.y)) {
            Refresh();
        }
    }
}

void BoardWindow::OnWheel(wxMouseEvent& e)
{
    if (e.GetWheelRotation() < 0) {
        if (g.current().redo()) {
            Refresh();
        }
    } else {
        if (g.current().undo()) {
            Refresh();
        }
    }
}

void BoardWindow::OnKeyDown(wxKeyEvent& e)
{
    const int key = e.GetKeyCode();
    Game& game = g.current();
    Gmode mode = game.get_mode();
    Point cur = game.rev_trans(g.board.get_cur());
    int cell;

    switch (key) {
        case 'A':
            if (mode == Gmode::CREATE) {
                game.change_to_answer_mode();
            } else if (mode == Gmode::SOLVE) {
                game.move_to_answer();
                Refresh();
            }
            break;

        case 'B':
        case 'W':
        case 'E':
            cell = CELL_SPACE;
            if (key == 'B') {
                cell = game.flip(CELL_BLACK);
            } else if (key == 'W') {
                cell = game.flip(CELL_WHITE);
            }

            if (game.put_stone(cell, cur.x, cur.y)) {
                Refresh();
            }
            break;

        case 'C':
            if (e.ShiftDown()) {
                game.remove_comment();
            } else {
                game.add_comment(frame->get_text());
            }
            break;

        case 'G':
            if (e.ShiftDown()) {
                if (g.prev_game()) {
                    Refresh();
                }
            } else {
                if (g.next_game()) {
                    Refresh();
                }
            }
            break;

        case 'L':
            {
                std::string s = frame->get_text();
                if (game.set_label(s, cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case 'P':
            if (mode == Gmode::ANSWER) {
                int cell = g.board.get_last_cell();
                if (cell == CELL_BLACK) {
                    cell = CELL_WHITE;
                } else if (cell == CELL_WHITE) {
                    cell = CELL_BLACK;
                }

                if (game.put_stone(cell, 20, 20)) {
                    Refresh();
                }
            }
            break;

        case 'K':
            if (game.change_to_free_mode()) {
                Refresh();
            }
            break;

        case 'Q':
            if (game.change_to_create_mode()) {
                Refresh();
            }
            break;

        case '@':
            if (game.toggle_correct()) {
                Refresh();
            }
            break;

        case '.':
            game.save();
            break;

        case '1':
            if (e.ShiftDown()) {
                if (game.set_label("1", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                if (game.set_mark(CELL_CR, cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case '2':
            if (e.ShiftDown()) {
                if (game.set_label("2", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                if (game.set_mark(CELL_MA, cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case '3':
            if (e.ShiftDown()) {
                if (game.set_label("3", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                if (game.set_mark(CELL_TR, cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case '4':
            if (e.ShiftDown()) {
                if (game.set_label("4", cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case '5':
            if (e.ShiftDown()) {
                if (game.set_label("5", cur.x, cur.y)) {
                    Refresh();
                }
            }
            break;

        case '6':
            if (e.ShiftDown()) {
                if (game.set_label("6", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                g.new_game(6);
                Refresh();
            }
            break;

        case '7':
            if (e.ShiftDown()) {
                if (game.set_label("7", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                g.new_game(9);
                Refresh();
            }
            break;

        case '8':
            if (e.ShiftDown()) {
                if (game.set_label("8", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                g.new_game(13);
                Refresh();
            }
            break;

        case '9':
            if (e.ShiftDown()) {
                if (game.set_label("9", cur.x, cur.y)) {
                    Refresh();
                }
            } else {
                g.new_game(19);
                Refresh();
            }
            break;

        case WXK_LEFT:
            if (game.undo()) {
                Refresh();
            }
            break;

        case WXK_RIGHT:
            if (game.redo()) {
                Refresh();
            }
            break;

        case WXK_DELETE:
            if (e.ControlDown()) {
                if (g.delete_game()) {
                    Refresh();
                }
            } else {
                if (game.delete_node()) {
                    Refresh();
                }
            }
            break;
    }
}
