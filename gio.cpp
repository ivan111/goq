#include <string>
#include "gio.h"
#include "g.h"
#include "node.h"

static void skip_spaces(std::ifstream& ifs)
{
    char ch;

    while (ifs.get(ch)) {
        if (std::isspace(ch)) {
            // skip
        } else {
            ifs.unget();
            return;
        }
    }
}

#define MAX_TOKEN_LEN (1023)

static char token[MAX_TOKEN_LEN+1];

Token get_token(std::ifstream& ifs)
{
    skip_spaces(ifs);

    char ch;
    int i = 0;

    while (ifs.get(ch)) {
        if (i >= MAX_TOKEN_LEN) {
            std::cerr << "load error: too long token" << std::endl;
            return Token::END;
        }

        token[i] = ch;
        i++;
        token[i] = '\0';

        if (i == 1) {
            if (ch == '(') {
                return Token::L_PAREN;
            }

            if (ch == ')') {
                return Token::R_PAREN;
            }

            if (ch == '[') {
                return Token::L_BRACKET;
            }

            if (ch == ']') {
                return Token::R_BRACKET;
            }

            if (ch == ';') {
                return Token::SEMICOLON;
            }
        } else if (ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == ';' || std::isspace(ch)) {
            ifs.unget();
            i--;
            token[i] = '\0';

            return Token::LABEL;
        }
    }

    return Token::END;
}

static Token get_prop_value(std::ifstream& ifs)
{
    int i = 0;
    char ch;
    bool is_escape = false;


    while (ifs.get(ch)) {
        if (i >= MAX_TOKEN_LEN) {
            std::cerr << "load error: too long token" << std::endl;
            return Token::END;
        }

        token[i] = ch;
        i++;
        token[i] = '\0';

        if (is_escape) {
            is_escape = false;

            if (ch == '\r') {
                i--;
                token[i] = '\0';

                if (ifs.get(ch)) {
                    if (ch == '\n') {
                        // skip
                    } else {
                        ifs.unget();
                    }
                }
            } else if (ch == '\n') {
                i--;
                token[i] = '\0';
            }

            continue;
        }

        if (ch == '\\') {
            is_escape = true;
        } else if (ch == ']') {
            i--;
            token[i] = '\0';
            return Token::R_BRACKET;
        }
    }

    std::cerr << "load error: not found ')'" << std::endl;

    return Token::END;
}

Property* load_property(std::ifstream& ifs)
{
    std::string id = std::string(token);

    Token t = get_token(ifs);
    if (t != Token::L_BRACKET) {
        std::cerr << "Error: not found '['" << std::endl;
        return nullptr;
    }

    t = get_prop_value(ifs);

    if (t == Token::END) {
        return nullptr;
    }

    std::list<std::string> vals;

    vals.push_back(token);

    char ch;
    skip_spaces(ifs);

    while (ifs.get(ch)) {
        if (ch == '[') {
            t = get_prop_value(ifs);

            if (t == Token::END) {
                return nullptr;
            }

            vals.push_back(token);
        } else {
            ifs.unget();
            break;
        }

        skip_spaces(ifs);
    }

    return new Property(id, vals);
}

// カッコ内のノードを読み込む
bool load_node(Route& route, std::ifstream& ifs)
{
    Token t = get_token(ifs);

    if (t != Token::SEMICOLON) {
        std::cerr << "load error: not found ';'" << std::endl;
        return false;
    }

    std::shared_ptr<Node> root(new Node(true));
    route.append(root);
    int depth = 1;

    t = get_token(ifs);

    while (t != Token::END) {
        if (t == Token::R_PAREN) {
            for (int i = 0; i < depth; i++) {
                route.visit_parent();
            }
            return true;
        }

        if (t == Token::SEMICOLON) {
            std::shared_ptr<Node> node(new Node(true));
            route.append(node);
            depth++;
        } else if (t == Token::L_PAREN) {
            bool result = load_node(route, ifs);

            if (!result) {
                return false;
            }
        } else {  // property
            for (char *tk = token; *tk; tk++) {
                if (!std::isupper(*tk)) {
                    std::cerr << "load error: unknown token " << token << std::endl;
                    return false;
                }
            }

            Property* p = load_property(ifs);

            if (!p) {
                return false;
            }

            std::shared_ptr<Property> prop(p);
            std::shared_ptr<Node> cur = route.current();
            cur->properties.push_back(std::move(prop));
        }

        t = get_token(ifs);
    }

    std::cerr << "load error: not found ')'" << std::endl;

    return false;
}
