#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <fstream>
#include "frame.h"
#include "board_window.h"
#include "node.h"
#include "g.h"

wxTreeCtrl* tree_ctrl;
wxTreeItemId root_id = nullptr;

wxImageList* image_list;

wxStaticText* pos_ctrl;
wxStaticText* s_comment_ctrl;
wxTextCtrl* d_text_ctrl;

MyFrame::MyFrame(G& g, std::string wrong)
    : wxFrame(nullptr, wxID_ANY, "goq"), g(g), wrong(wrong)
{
    wxMenu *file = new wxMenu;

    file->Append(ID_NEW6, wxT("&New(6)"));
    file->Append(ID_NEW9, wxT("&New(9)"));
    file->Append(ID_NEW13, wxT("&New(13)"));
    file->Append(ID_NEW19, wxT("&New(19)"));
    file->Append(ID_OPEN, wxT("&Open"));
    file->Append(ID_SAVE, wxT("&Save"));
    file->AppendSeparator();
    file->Append(wxID_EXIT);

    wxMenuBar *menuBar = new wxMenuBar;
    menuBar->Append(file, "&File");
    SetMenuBar(menuBar);

    Bind(wxEVT_MENU, &MyFrame::OnNew, this, ID_NEW6);
    Bind(wxEVT_MENU, &MyFrame::OnNew, this, ID_NEW9);
    Bind(wxEVT_MENU, &MyFrame::OnNew, this, ID_NEW13);
    Bind(wxEVT_MENU, &MyFrame::OnNew, this, ID_NEW19);
    Bind(wxEVT_MENU, &MyFrame::OnOpen, this, ID_OPEN);
    Bind(wxEVT_MENU, &MyFrame::OnSave, this, ID_SAVE);
    Bind(wxEVT_MENU, &MyFrame::OnExit, this, wxID_EXIT);

    CreateStatusBar();
    create_controls();
}

void MyFrame::create_controls()
{
    BoardWindow* win = new BoardWindow(this, g);

    tree_ctrl = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_LINES_AT_ROOT | wxTR_HAS_BUTTONS);
    root_id = tree_ctrl->AddRoot("Root");
    tree_ctrl->SetIndent(30);

    image_list = new wxImageList(16, 16);
    tree_ctrl->SetImageList(image_list);
    for (int i = 0; i <= 8; i++) {
        wxImage img = wxImage("res/" + std::to_string(i) + ".bmp");
        img.SetMaskColour(255, 255, 255);
        image_list->Add(wxBitmap(img));
    }

    pos_ctrl = new wxStaticText(this, wxID_ANY, "");

    s_comment_ctrl = new wxStaticText(this, wxID_ANY, "");
    d_text_ctrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);

    Gmode mode = g.current().get_mode();
    if (mode == Gmode::SOLVE || mode == Gmode::KIFU) {
        d_text_ctrl->Hide();
    }


    wxBoxSizer* h_sizer = new wxBoxSizer(wxHORIZONTAL);
    wxBoxSizer* lv_sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* rv_sizer = new wxBoxSizer(wxVERTICAL);

    lv_sizer->Add(win, 10, wxALL | wxEXPAND, 5);
    lv_sizer->Add(s_comment_ctrl, 1, wxALL | wxEXPAND, 5);
    lv_sizer->Add(d_text_ctrl, 1, wxALL | wxEXPAND, 5);

    rv_sizer->Add(pos_ctrl, 0, wxALL, 5);
    rv_sizer->Add(tree_ctrl, 1, wxALL | wxEXPAND, 5);

    h_sizer->Add(lv_sizer, 1, wxALL | wxEXPAND, 5);
    h_sizer->Add(rv_sizer, 1, wxALL | wxEXPAND, 5);

    SetSizer(h_sizer);


    g.add_listener(this);
    on_comment(g.get_comment());
    on_info(g.get_info());
    on_pos(g.get_pos());
}

std::string MyFrame::get_text()
{
    wxString text = d_text_ctrl->GetValue();
    return std::string(text.mb_str(wxConvUTF8));
}

void MyFrame::OnExit(wxCommandEvent& event)
{
    (void)event;

    Close(true);
}

void MyFrame::OnNew(wxCommandEvent& event)
{
    int size = 13;
    int id = event.GetId();

    if (id == ID_NEW6) {
        size = 6;
    } else if (id == ID_NEW9) {
        size = 9;
    } else if (id == ID_NEW13) {
        size = 13;
    } else if (id == ID_NEW19) {
        size = 19;
    }

    g.new_game(size);

    Refresh();
}

void create_tree_sub(Game& g, wxTreeItemId parent_id, std::shared_ptr<Node> node)
{
    wxTreeItemId id = tree_ctrl->AppendItem(parent_id, wxString::FromUTF8(node->to_string(g)));

    Route route = g.get_route();

    int cell = CELL_SPACE;
    if (node->has(PID::B)) {
        cell = CELL_BLACK;
    } else if (node->has(PID::W)) {
        cell = CELL_WHITE;
    }

    cell = g.flip(cell);

    bool is_cur = (route.current() == node);
    bool in_route = route.exists(node);
    int image_no = 0;

    if (cell == CELL_SPACE) {
        if (is_cur) {
            image_no = 1;
        } else if (in_route) {
            image_no = 2;
        } else {
            image_no = 0;
        }
    } else if (cell == CELL_BLACK) {
        if (is_cur) {
            image_no = 4;
        } else if (in_route) {
            image_no = 5;
        } else {
            image_no = 3;
        }
    } else {
        if (is_cur) {
            image_no = 7;
        } else if (in_route) {
            image_no = 8;
        } else {
            image_no = 6;
        }
    }

    if (in_route) {
        tree_ctrl->EnsureVisible(id);
    }

    tree_ctrl->SetItemImage(id, image_no);

    if (node->children.empty()) {
        return;
    }

    if (node->children.size() >= 2) {
        int i = 0;
        for (auto n : node->children) {
            std::string s = "---- ";
            s += 'A' + i;
            s += " ----";
            tree_ctrl->AppendItem(id, s);

            create_tree_sub(g, id, n);

            i++;
        }

        return;
    }

    create_tree_sub(g, parent_id, *node->children.begin());
}

void MyFrame::on_tree(Game& g)
{
    tree_ctrl->DeleteChildren(root_id);

    create_tree_sub(g, root_id, g.get_root());
}

void MyFrame::OnOpen(wxCommandEvent& event)
{
    (void)event;

    wxLogMessage("Hello world from wxWidgets!");
}

void MyFrame::OnSave(wxCommandEvent& event)
{
    (void)event;

    wxLogMessage("Hello world from wxWidgets!");
}

void MyFrame::SaveFileAs(wxCommandEvent& WXUNUSED(event))
{
    wxFileDialog *SaveDialog = new wxFileDialog(
            this, _("Save File As _?"), wxEmptyString, wxEmptyString,
            _("Text files (*.txt)|*.txt|SGF Files (*.sgf)|*.sgf"),
            wxFD_SAVE | wxFD_OVERWRITE_PROMPT, wxDefaultPosition);

    if (SaveDialog->ShowModal() == wxID_OK) {
        /*
        CurrentDocPath = SaveDialog->GetPath();
        MainEditBox->SaveFile(CurrentDocPath);

        SetTitle(wxString("Edit - ") << SaveDialog->GetFilename());
        */
    }

    SaveDialog->Destroy();
}

void MyFrame::on_comment(std::string comment)
{
    s_comment_ctrl->SetLabel(wxString::FromUTF8(comment));
}

void MyFrame::on_pos(std::string pos)
{
    pos_ctrl->SetLabel(pos);
}

void MyFrame::on_wrong(std::string sgf)
{
    if (wrong != "") {
        std::ofstream ofs(wrong, std::ios::app);
        ofs << sgf << std::endl;
    }
}

void MyFrame::on_info(std::string info)
{
    SetStatusText(wxString::FromUTF8(info));
}
