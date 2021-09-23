#ifndef FRAME_H
#define FRAME_H

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "g.h"

enum
{
    ID_NEW6 = 1,
    ID_NEW9,
    ID_NEW13,
    ID_NEW19,
    ID_OPEN,
    ID_SAVE,
};

class MyFrame : public wxFrame, public GEventListener
{
    G& g;
    std::string wrong;
public:
    MyFrame(G& g, std::string wrong);

    std::string get_text();

    virtual void on_comment(std::string comment);
    virtual void on_pos(std::string pos);
    virtual void on_wrong(std::string sgf);
    virtual void on_info(std::string info);
    virtual void on_tree(Game& g);
private:
    void create_controls();

    void OnNew(wxCommandEvent& event);
    void OnOpen(wxCommandEvent& event);
    void OnSave(wxCommandEvent& event);
    void OnExit(wxCommandEvent& event);
    void OnAbout(wxCommandEvent& event);

    void SaveFileAs(wxCommandEvent& event);
};

#endif
