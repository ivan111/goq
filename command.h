#ifndef COMMAND_H
#define COMMAND_H

#include <list>
#include <map>
#include <string>
#include "board.h"
#include "g.h"

class Command
{
public:
    virtual ~Command() {};

    virtual bool exec(G& g) { (void)g; return false; };
    virtual void undo(G& g) {(void)g;};
    virtual void redo(G& g) {(void)g;};
};


class MakeMoveCmd : public Command
{
    int cell, x, y;
    int old_cell;
    std::list<Move> hama;
    bool done = false;

    Point old_kou;
    Point new_kou;

    Point old_last;
    Point new_last;

    bool old_is_pass;
    bool new_is_pass;

    int old_pass_stone;
    int new_pass_stone;

    int old_n_moves;
    int new_n_moves;

    int old_n_black_hama;
    int new_n_black_hama;

    int old_n_white_hama;
    int new_n_white_hama;
public:
    MakeMoveCmd(int cell, int x, int y);
    virtual bool exec(G& g);
    virtual void undo(G& g);
    virtual void redo(G& g);
};

using BinOpPtr = int (*)(int, int);

class BinOpCmd : public Command
{
    BinOpPtr fn;
    std::list<Move> moves;
    std::list<Move> old_moves;
public:
    BinOpCmd(BinOpPtr fn, std::list<Move> moves);
    virtual bool exec(G& g);
    virtual void undo(G& g);
    virtual void redo(G& g);
};

class CommentCmd : public Command
{
    std::string comment;
    std::string old_comment;
public:
    CommentCmd(std::string comment)
        : comment(comment) {};
    virtual bool exec(G& g);
    virtual void undo(G& g);
    virtual void redo(G& g);
};


class SizeCmd : public Command
{
    int size;
public:
    SizeCmd(int size) : size(size) {};
    virtual bool exec(G& g) { g.board.init(size); return true; };
    virtual void undo(G& g) {(void)g;};
    virtual void redo(G& g) { g.board.init(size); };
};


#endif
