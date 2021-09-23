#include "command.h"

bool CommentCmd::exec(G& g)
{
    old_comment = g.set_comment(comment);

    return true;
}

void CommentCmd::undo(G& g)
{
    g.set_comment(old_comment);
}

void CommentCmd::redo(G& g)
{
    g.set_comment(comment);
}

MakeMoveCmd::MakeMoveCmd(int cell, int x, int y)
    : cell(cell), x(x), y(y)
{
}

bool MakeMoveCmd::exec(G& g)
{
    if (done) {
        redo(g);
        return true;
    }

    if (x == 20 && y == 20) {
        // pass
    } else if (g.board.is_out(x, y)) {
        return false;
    }

    old_kou = g.board.get_kou();
    old_last = g.board.get_last();
    old_cell = g.board.get_cell(x, y);
    old_is_pass = g.board.is_pass;
    old_pass_stone = g.board.pass_stone;
    old_n_moves = g.board.n_moves;
    old_n_black_hama = g.board.n_black_hama;
    old_n_white_hama = g.board.n_white_hama;

    bool result = g.board.make_move(cell, x, y, hama);

    new_kou = g.board.get_kou();
    new_last = g.board.get_last();
    new_is_pass = g.board.is_pass;
    new_pass_stone = g.board.pass_stone;
    new_n_moves = g.board.n_moves;
    new_n_black_hama = g.board.n_black_hama;
    new_n_white_hama = g.board.n_white_hama;

    if (result == true) {
        done = true;
    }

    return result;
}

void MakeMoveCmd::undo(G& g)
{
    if (!new_is_pass && g.board.is_out(x, y)) {
        return;
    }

    if (!new_is_pass) {
        g.board.set_cell(old_cell, x, y);
    }
    g.board.get_kifu().pop_back();
    g.board.set_kou(old_kou);
    g.board.set_last(old_last);
    g.board.is_pass = old_is_pass;
    g.board.pass_stone = old_pass_stone;
    g.board.n_moves = old_n_moves;
    g.board.n_black_hama = old_n_black_hama;
    g.board.n_white_hama = old_n_white_hama;

    for (auto m : hama) {
        g.board.set_cell(m.cell, m.x, m.y);
    }
}

void MakeMoveCmd::redo(G& g)
{
    if (!new_is_pass && g.board.is_out(x, y)) {
        return;
    }

    if (!new_is_pass) {
        g.board.set_cell(cell, x, y);
    }
    g.board.get_kifu().push_back(Move(cell & 3, x, y));
    g.board.set_kou(new_kou);
    g.board.set_last(new_last);
    g.board.is_pass = new_is_pass;
    g.board.pass_stone = new_pass_stone;
    g.board.n_moves = new_n_moves;
    g.board.n_black_hama = new_n_black_hama;
    g.board.n_white_hama = new_n_white_hama;

    for (auto m : hama) {
        g.board.set_cell(CELL_SPACE, m.x, m.y);
    }
}

BinOpCmd::BinOpCmd(BinOpPtr fn, std::list<Move> moves)
    : fn(fn), moves(moves)
{
}

bool BinOpCmd::exec(G& g)
{
    if (moves.empty()) {
        return false;
    }

    bool result = false;

    for (auto m : moves) {
        if (g.board.is_out(m.x, m.y)) {
            continue;
        }

        int old = g.board.get_cell(m.x, m.y);
        int cell = fn(old, m.cell);

        g.board.set_cell(cell, m.x, m.y);

        if (cell == old) {
            continue;
        }

        result = true;

        old_moves.push_back(Move(old, m.x, m.y));
    }

    return result;
}

void BinOpCmd::undo(G& g)
{
    for (auto m : old_moves) {
        g.board.set_cell(m.cell, m.x, m.y);
    }
}

void BinOpCmd::redo(G& g)
{
    exec(g);
}
