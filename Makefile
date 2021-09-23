CC = g++
CPPFLAGS = -std=c++11 -Wall -Wextra -Werror
WX_CPPFLAGS = -I/usr/local/lib/wx/include/gtk3-unicode-3.1 -I/usr/local/include/wx-3.1 -D_FILE_OFFSET_BITS=64 -DWXUSINGDLL -D__WXGTK__ -pthread -L/usr/local/lib -pthread   -lwx_gtk3u_xrc-3.1 -lwx_gtk3u_html-3.1 -lwx_gtk3u_qa-3.1 -lwx_gtk3u_core-3.1 -lwx_baseu_xml-3.1 -lwx_baseu_net-3.1 -lwx_baseu-3.1

goq: frame.o board.o command.o node.o gio.o g.o board_window.o main.o
	$(CC) $(CPPFLAGS) $(WX_CPPFLAGS) -o $@ $^

test: test.o board.o command.o node.o gio.o g.o
	$(CC) $(CPPFLAGS) -o $@ $^


main.o: main.cpp
	$(CC) $(CPPFLAGS) $(WX_CPPFLAGS) -c main.cpp

frame.o: frame.cpp frame.h
	$(CC) $(CPPFLAGS) $(WX_CPPFLAGS) -c frame.cpp

board_window.o: board_window.cpp board_window.h
	$(CC) $(CPPFLAGS) $(WX_CPPFLAGS) -c board_window.cpp


test.o: test.cpp
	$(CC) $(CPPFLAGS) -c test.cpp

board.o: board.cpp board.h
	$(CC) $(CPPFLAGS) -c board.cpp

command.o: command.cpp command.h
	$(CC) $(CPPFLAGS) -c command.cpp

node.o: node.cpp node.h
	$(CC) $(CPPFLAGS) -c node.cpp

gio.o: gio.cpp gio.h
	$(CC) $(CPPFLAGS) -c gio.cpp

g.o: g.cpp g.h
	$(CC) $(CPPFLAGS) -c g.cpp


.PHONY: clean
clean:
	-rm main.o frame.o board_window.o test.o board.o command.o node.o gio.o g.o
