#include <wx/wxprec.h>
#ifndef WX_PRECOMP
    #include <wx/wx.h>
    #include <wx/cmdline.h>
#endif

#include "g.h"
#include "frame.h"

class MyApp : public wxApp
{
    MyFrame *frame;

    std::list<std::string> m_files;
    std::string m_wrong;
    std::string m_kifu;
public:
    virtual bool OnInit();
    virtual void OnInitCmdLine(wxCmdLineParser& parser);
    virtual bool OnCmdLineParsed(wxCmdLineParser& parser);
};

wxIMPLEMENT_APP(MyApp);

bool MyApp::OnInit()
{
    if (!wxApp::OnInit())
        return false;

    srand((unsigned int)time(NULL));

    G* g = new G();

    frame = new MyFrame(*g, m_wrong);

    if (m_kifu != "") {
        g->load(Gmode::KIFU, m_kifu);
    } else {
        bool is_append = false;
        for (auto f : m_files) {
            if (g->load(Gmode::SOLVE, f, is_append)) {
                is_append = true;
            }
        }
    }

    if (g->current().get_mode() != Gmode::SOLVE) {
        frame->on_tree(g->current());
    }

    frame->Show(true);

    return true;
}

void MyApp::OnInitCmdLine(wxCmdLineParser& parser)
{
    parser.SetSwitchChars (wxT("-"));
    parser.AddParam("sgf", wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE);
    parser.AddOption("w", "wrong", "", wxCMD_LINE_VAL_STRING);
    parser.AddOption("k", "kifu", "", wxCMD_LINE_VAL_STRING);
}

bool MyApp::OnCmdLineParsed(wxCmdLineParser& parser)
{
    for (int i = 0; i < (int) parser.GetParamCount(); i++)
    {
        wxString s = parser.GetParam(i);
        m_files.push_back(std::string(s.mb_str()));
    }

    wxString wx_wrong;
    if (parser.Found("w", &wx_wrong)) {
        std::string wrong = std::string(wx_wrong.mb_str());

        struct stat s;

        if (stat(wrong.c_str(), &s) != 0) { // 存在しない
            m_wrong = wrong;
        }
    }

    wxString wx_kifu;
    if (parser.Found("k", &wx_kifu)) {
        std::string kifu = std::string(wx_kifu.mb_str());

        struct stat s;

        if (stat(kifu.c_str(), &s) == 0) {  // 存在する
            if (s.st_mode & S_IFDIR) {
                // 何もしない
            } else {
                m_kifu = kifu;
            }
        }
    }

    return true;
}
