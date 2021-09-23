#ifndef BOARD_WINDOW_H
#define BOARD_WINDOW_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "g.h"

class MyFrame;

#define MIN_WIDTH (21 * 30)

class BoardWindow : public wxWindow
{
    int width = MIN_WIDTH;
    G& g;
    MyFrame* frame;

public:
    BoardWindow(MyFrame* frame, G& g);

    void OnSize(wxSizeEvent& event);
    void OnPaint(wxPaintEvent& event);

    void OnKeyDown(wxKeyEvent& event);

    void OnMove(wxMouseEvent& event);
    void OnEnter(wxMouseEvent& event);
    void OnLeave(wxMouseEvent& event);
    void OnClick(wxMouseEvent& event);
    void OnWheel(wxMouseEvent& event);
};

#endif
