#ifndef GIO_H
#define GIO_H

#include <fstream>

class Route;

enum class Token {
    L_PAREN,
    R_PAREN,
    L_BRACKET,
    R_BRACKET,
    SEMICOLON,
    LABEL,
    END,
};

Token get_token(std::ifstream& ifs);
bool load_node(Route& route, std::ifstream& ifs);

#endif
