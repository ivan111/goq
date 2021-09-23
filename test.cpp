#include <iostream>
#include "command.h"
#include "g.h"

void create_tree(G& g) {
    (void)g;
}

void test_load_sgf()
{
    G g;
    if (!g.load("a.sgf")) {
        return;
    }

    std::cout << g.board << std::endl;
    g.current().print_nodes();
}

void test_board()
{
    G g;

    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_BLACK, 1, 1);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_WHITE, 2, 1);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_BLACK, 3, 1);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_WHITE, 1, 2);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_BLACK, 2, 2);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_WHITE, 3, 3);
    std::cout << g.board << std::endl;
    g.current().put_stone(CELL_BLACK, 1, 1);
    std::cout << g.board << std::endl;
}

int main()
{
    test_board();
    test_load_sgf();

    return 0;
}
