//
// $Source$
// $Date$
// $Revision$
//
// DESCRIPTION:
// Implementation of view of normal form in matrix format
//
// This file is part of Gambit
// Copyright (c) 2004, The Gambit Project
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//

#include <wx/wxprec.h>
#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif  // WX_PRECOMP

#include "table-matrix.h"

#include "sheet.h"     // the wxSheet widget

// We hard-code player colors for now; these should be properties
// of the game document, and user-configurable.
static wxColour s_colors[8] =
  { wxColour(255, 0, 0),
    wxColour(0, 0, 255),
    wxColour(0, 128, 0),
    wxColour(255, 128, 0),
    wxColour(0, 0, 64),
    wxColour(128, 0, 255),
    wxColour(64, 0, 0),
    wxColour(255, 128, 255)
  };

//-----------------------------------------------------------------------
//                  class gbtPlayerLabelCtrl
//-----------------------------------------------------------------------

class gbtPlayerLabelCtrl : public wxScrolledWindow {
private:
  gbtTableMatrix *m_view;
  bool m_rowPlayer;
  wxMenu *m_popupMenu;

  void OnDraw(wxDC &);

  void OnRightDown(wxMouseEvent &);
  void OnEnterWindow(wxMouseEvent &);
  void OnLeaveWindow(wxMouseEvent &);
  void OnMenu(wxCommandEvent &);

public:
  gbtPlayerLabelCtrl(gbtTableMatrix *p_parent, bool p_rowPlayer);
  virtual ~gbtPlayerLabelCtrl();

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(gbtPlayerLabelCtrl, wxScrolledWindow)
  EVT_RIGHT_DOWN(gbtPlayerLabelCtrl::OnRightDown)
  EVT_MENU_RANGE(1000, 32767, gbtPlayerLabelCtrl::OnMenu)
  EVT_ENTER_WINDOW(gbtPlayerLabelCtrl::OnEnterWindow)
  EVT_LEAVE_WINDOW(gbtPlayerLabelCtrl::OnLeaveWindow)
END_EVENT_TABLE()

gbtPlayerLabelCtrl::gbtPlayerLabelCtrl(gbtTableMatrix *p_view,
				       bool p_rowPlayer)
  : wxScrolledWindow(p_view, -1),
    m_view(p_view), m_rowPlayer(p_rowPlayer),
    m_popupMenu(0)
{
  SetBackgroundColour(wxColour(250, 250, 250));
  if (p_rowPlayer) {
    SetToolTip(_("The player choosing the row"));
  }
  else {
    SetToolTip(_("The player choosing the column"));
  }
}

gbtPlayerLabelCtrl::~gbtPlayerLabelCtrl()
{
  if (m_popupMenu)  delete m_popupMenu;
}

void gbtPlayerLabelCtrl::OnDraw(wxDC &p_dc)
{
  p_dc.SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD));
  wxCoord width, height;

  wxString label;
  wxColour color;
  if (m_rowPlayer) {
    label = m_view->GetDocument()->GetGame()->GetPlayer(m_view->GetRowPlayer())->GetLabel().c_str();
    color = s_colors[(m_view->GetRowPlayer() - 1) % 8];
  }
  else {
    label = m_view->GetDocument()->GetGame()->GetPlayer(m_view->GetColPlayer())->GetLabel().c_str();
    color = s_colors[(m_view->GetColPlayer() - 1) % 8];
  }
  
  p_dc.GetTextExtent(label, &width, &height);
  p_dc.SetTextForeground(color);
  wxSize size = GetClientSize();

  if (m_rowPlayer) {
    p_dc.DrawRotatedText(label, 
			 (size.GetWidth() - height) / 2,
			 (size.GetHeight() + width) / 2,
			 90);
  }
  else {
    p_dc.DrawText(label, 
		  (size.GetWidth() - width) / 2,
		  (size.GetHeight() - height) / 2);
  }
}

void gbtPlayerLabelCtrl::OnRightDown(wxMouseEvent &p_event)
{
  if (m_popupMenu)  delete m_popupMenu;

  m_popupMenu = new wxMenu("Show player on this axis");
  const gbtGame &game = m_view->GetDocument()->GetGame();
  for (int pl = 1; pl <= game->NumPlayers(); pl++) {
    m_popupMenu->Append(1000 + pl,
			wxString::Format("%d: %s",
					 pl, 
					 game->GetPlayer(pl)->GetLabel().c_str()));
  }

  PopupMenu(m_popupMenu, p_event.GetX(), p_event.GetY());
}

void gbtPlayerLabelCtrl::OnMenu(wxCommandEvent &p_event)
{
  int newPlayer = p_event.GetId() - 1000;

  if (m_rowPlayer) {
    if (newPlayer == m_view->GetColPlayer()) {
      m_view->SetPlayers(m_view->GetColPlayer(), m_view->GetRowPlayer());
    }
    else {
      m_view->SetPlayers(p_event.GetId() - 1000, m_view->GetColPlayer());
    }
  }
  else {
    if (newPlayer == m_view->GetRowPlayer()) {
      m_view->SetPlayers(m_view->GetColPlayer(), m_view->GetRowPlayer());
    }
    else {
      m_view->SetPlayers(m_view->GetRowPlayer(), p_event.GetId() - 1000);
    }
  }
}

void gbtPlayerLabelCtrl::OnEnterWindow(wxMouseEvent &)
{
  SetBackgroundColour(wxColour(210, 210, 210));
  Refresh();
}

void gbtPlayerLabelCtrl::OnLeaveWindow(wxMouseEvent &)
{
  SetBackgroundColour(wxColour(250, 250, 250));
  Refresh();
}

class gbtTablePlayerCtrl : public wxScrolledWindow {
private:
  gbtTableMatrix *m_view;
  int m_index;
  wxMenu *m_popupMenu;

  void OnDraw(wxDC &);

  void OnRightDown(wxMouseEvent &);
  void OnEnterWindow(wxMouseEvent &);
  void OnLeaveWindow(wxMouseEvent &);
  void OnMenu(wxCommandEvent &);

public:
  gbtTablePlayerCtrl(wxWindow *p_parent, gbtTableMatrix *p_view,
		     int p_index);
  virtual ~gbtTablePlayerCtrl();

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(gbtTablePlayerCtrl, wxScrolledWindow)
  EVT_RIGHT_DOWN(gbtTablePlayerCtrl::OnRightDown)
  EVT_MENU_RANGE(1000, 32767, gbtTablePlayerCtrl::OnMenu)
  EVT_ENTER_WINDOW(gbtTablePlayerCtrl::OnEnterWindow)
  EVT_LEAVE_WINDOW(gbtTablePlayerCtrl::OnLeaveWindow)
END_EVENT_TABLE()

gbtTablePlayerCtrl::gbtTablePlayerCtrl(wxWindow *p_parent,
				       gbtTableMatrix *p_view,
				       int p_index)
  : wxScrolledWindow(p_parent, -1),
    m_view(p_view), m_index(p_index), m_popupMenu(0)
{
  SetBackgroundColour(wxColour(250, 250, 250));
  SetToolTip(_("A player choosing the table")); 
}

gbtTablePlayerCtrl::~gbtTablePlayerCtrl()
{
  if (m_popupMenu)  delete m_popupMenu;
}

void gbtTablePlayerCtrl::OnDraw(wxDC &p_dc)
{
  p_dc.SetFont(wxFont(10, wxSWISS, wxNORMAL, wxBOLD));
  wxCoord width, height;

  int player = m_view->GetTablePlayer(m_index);
  int strat = m_view->GetStrategy(player);
  const gbtGame &game = m_view->GetDocument()->GetGame();
  wxString label = (game->GetPlayer(player)->GetLabel() +
		    " plays strategy " +
		    game->GetPlayer(player)->GetStrategy(strat)->GetLabel()).c_str();
  wxColour color = s_colors[(player - 1) % 8];
  
  p_dc.GetTextExtent(label, &width, &height);
  p_dc.SetTextForeground(color);
  wxSize size = GetClientSize();

  p_dc.DrawText(label, 
		(size.GetWidth() - width) / 2,
		(size.GetHeight() - height) / 2);
}

void gbtTablePlayerCtrl::OnRightDown(wxMouseEvent &p_event)
{
  if (m_popupMenu)  delete m_popupMenu;

  gbtGamePlayer player = 
    m_view->GetDocument()->GetGame()->GetPlayer(m_view->GetTablePlayer(m_index));

  m_popupMenu = new wxMenu(wxString::Format("Set displayed strategy for %s",
					    player->GetLabel().c_str()));

  for (int st = 1; st <= player->NumStrategies(); st++) {
    m_popupMenu->Append(1000 + st,
			wxString::Format("%d: %s", st,
					 player->GetStrategy(st)->GetLabel().c_str()));
  }

  PopupMenu(m_popupMenu, p_event.GetX(), p_event.GetY());
}

void gbtTablePlayerCtrl::OnMenu(wxCommandEvent &p_event)
{
  int newStrategy = p_event.GetId() - 1000;
  int player = m_view->GetTablePlayer(m_index);
  m_view->SetStrategy(player, newStrategy);
}

void gbtTablePlayerCtrl::OnEnterWindow(wxMouseEvent &)
{
  SetBackgroundColour(wxColour(210, 210, 210));
  Refresh();
}

void gbtTablePlayerCtrl::OnLeaveWindow(wxMouseEvent &)
{
  SetBackgroundColour(wxColour(250, 250, 250));
  Refresh();
}

//-----------------------------------------------------------------------
//                  class gbtTableChoiceCtrl
//-----------------------------------------------------------------------

class gbtTableChoiceCtrl : public wxPanel {
private:
  gbtTableMatrix *m_parent;
  gbtTablePlayerCtrl **m_players;

  void OnPaint(wxPaintEvent &);
  void OnSize(wxSizeEvent &);
  
public:
  gbtTableChoiceCtrl(gbtTableMatrix *p_parent);

  int GetBestHeight(void) const;

  DECLARE_EVENT_TABLE()
};

BEGIN_EVENT_TABLE(gbtTableChoiceCtrl, wxPanel)
  EVT_PAINT(gbtTableChoiceCtrl::OnPaint)
  EVT_SIZE(gbtTableChoiceCtrl::OnSize)
END_EVENT_TABLE()

gbtTableChoiceCtrl::gbtTableChoiceCtrl(gbtTableMatrix *p_parent)
  : wxPanel(p_parent, -1), m_parent(p_parent)
{
  m_players = new gbtTablePlayerCtrl *[m_parent->GetDocument()->GetGame()->NumPlayers() - 2];
  for (int pl = 0; pl < m_parent->GetDocument()->GetGame()->NumPlayers() - 2; pl++) {
    m_players[pl] = new gbtTablePlayerCtrl(this, p_parent, pl + 1);
  }
  SetToolTip(_("Other players' choices determining the displayed table"));
}

void gbtTableChoiceCtrl::OnPaint(wxPaintEvent &)
{
  wxPaintDC dc(this);

  dc.SetBrush(wxBrush(*wxWHITE, wxSOLID));
  dc.SetPen(wxPen(*wxBLACK, 1, wxSOLID));

  dc.DrawRectangle(0, 0, 
		   GetClientSize().GetWidth(), GetClientSize().GetHeight());
  dc.DrawRectangle(0, 0, 100, GetClientSize().GetHeight());

  dc.SetFont(wxFont(10, wxSWISS, wxNORMAL, wxBOLD));
  wxCoord textWidth, textHeight;
  dc.GetTextExtent("Table", &textWidth, &textHeight);
  
  dc.DrawText("Table",
	      (100 - textWidth) / 2,
	      GetClientSize().GetHeight() / 2 - textHeight);
  dc.GetTextExtent("Choices:", &textWidth, &textHeight);
  dc.DrawText("Choices:",
	      (100 - textWidth) / 2,
	      GetClientSize().GetHeight() / 2);
}

void gbtTableChoiceCtrl::OnSize(wxSizeEvent &)
{
  for (int pl = 0; pl < m_parent->GetDocument()->GetGame()->NumPlayers() - 2; pl++) {
    m_players[pl]->SetSize(100, (pl * 20) + 1, 
			   GetClientSize().GetWidth() - 101, 20); 
  }
}

int gbtTableChoiceCtrl::GetBestHeight(void) const
{
  int pl = m_parent->GetDocument()->GetGame()->NumPlayers() - 2;
  if (pl < 2)  pl = 2;
  return pl * 20 + 2;
}


class gbtMatrixSheet : public wxSheet, public gbtGameView {
private:
  gbtTableMatrix *m_view;

  // Overriding wxSheet members for data access
  wxString GetCellValue(const wxSheetCoords &);
  void SetCellValue(const wxSheetCoords &, const wxString &);
  wxSheetCellAttr GetAttr(const wxSheetCoords &p_coords, wxSheetAttr_Type) const;

  // Overriding wxSheet members for customized behavior
  wxRect CellToRect(const wxSheetCoords &, bool = false) const;
  void DrawCellBorder(wxDC &, const wxSheetCoords &);
  void DrawColLabels(wxDC &, const wxArrayInt &);

public:
  gbtMatrixSheet(gbtTableMatrix *m_view);

  // Implementation of gbtGameView members
  void OnUpdate(void);
};


gbtMatrixSheet::gbtMatrixSheet(gbtTableMatrix *p_view)
  : wxSheet(p_view, -1), gbtGameView(p_view->GetDocument()),
    m_view(p_view)
{
  CreateGrid(m_doc->GetGame()->GetPlayer(m_view->GetRowPlayer())->NumStrategies(),
	     m_doc->GetGame()->GetPlayer(m_view->GetColPlayer())->NumStrategies() *
	     m_doc->GetGame()->NumPlayers());

  EnableEditing(true);
  DisableDragRowSize();
  DisableDragColSize();
  SetCursorCellHighlightColour(*wxWHITE);

  AutoSizeRows();
  AutoSizeColumns();
  for (int col = 0; col <= GetNumberCols(); col++) {
    if (GetColWidth(col) < GetRowHeight(col)) {
      SetColWidth(col, GetRowHeight(col));
    }
  }
  AdjustScrollbars();
}

static bool wxRectIsEmpty(const wxRect &rect)
{
  return (rect.width < 1) || (rect.height < 1);
}

void gbtMatrixSheet::DrawCellBorder( wxDC& dc, const wxSheetCoords& coords )
{
  wxRect rect(CellToRect( coords ));
  if ( wxRectIsEmpty(rect) )  // !IsCellShown
    return;

  dc.SetPen( wxPen(GetGridLineColour(), 1, wxSOLID) );
  // right hand border
  // the right border gets drawn only for the last column
  if ((coords.GetCol() + 1) % m_doc->GetGame()->NumPlayers() == 0) {
    dc.DrawLine( rect.x + rect.width, rect.y,
		 rect.x + rect.width, rect.y + rect.height + 1 );
  }

  // bottom border
  dc.DrawLine( rect.x - 1,          rect.y + rect.height,
	       rect.x + rect.width, rect.y + rect.height);
}

//
// This overrides cell rectangle calculations for column labels
//
wxRect gbtMatrixSheet::CellToRect(const wxSheetCoords &p_coords,
			    bool p_getDeviceRect) const
{
  if (IsColLabelCell(p_coords)) {
    int virtcol = p_coords.GetCol() - (p_coords.GetCol() % m_doc->GetGame()->NumPlayers());
    wxSheetBlock block(-1, virtcol, 1, m_doc->GetGame()->NumPlayers());

    wxRect rect(GetColLeft(block.GetLeft()), GetRowTop(block.GetTop()), 0, 0);
    rect.width  = GetColRight( block.GetRight() ) - rect.x; // + 1;
    rect.height = GetRowBottom(block.GetBottom()) - rect.y; // + 1;
    
    if (GridLinesEnabled()) {
      rect.width--;
      rect.height--;
    }
    
    int dummy;
    CalcScrolledPosition(0, rect.y, &dummy, &rect.y);
    return rect;

  }
  else {
    return wxSheet::CellToRect(p_coords, p_getDeviceRect);
  }
}

// This is identical to the wxSheet implementation, except the
// right sides of only odd-numbered columns are drawn
void gbtMatrixSheet::DrawColLabels( wxDC& dc, const wxArrayInt& cols )
{
  size_t i, numLabels = cols.GetCount();
  if (GetNumberCols() == 0 || !numLabels ) return;

  for ( i = 0; i < numLabels; i++ )
    DrawCell( dc, wxSheetCoords(-1, cols[i]) );

  // Now draw the dividing lines
  dc.SetPen(wxPen(GetLabelOutlineColour(), 1, wxSOLID));
  int left, dummy;
  CalcUnscrolledPosition(0, 0, &left, &dummy);
  int right = GetColRight(cols[numLabels-1]); 
  // top of col labels
  dc.DrawLine(left, 0, right, 0);
  // bottom of col labels
  dc.DrawLine(left, GetColLabelHeight()-1, right, GetColLabelHeight()-1);
  // draw rights
  for (i = 0; i < numLabels; i++) {
    if ((cols[i] + 1) % m_doc->GetGame()->NumPlayers() == 0) {
      right = GetColRight(cols[i])-1;
      dc.DrawLine(right, 0, right, GetColLabelHeight()-1);
    }
  }
}

//
// For Matrix-style payoffs, each strategy corresponds to a 2x2 block
// of cells.  The upper-right cell shows the column player payoffs,
// and the lower-left cell shows the row player payoffs.
//
wxString gbtMatrixSheet::GetCellValue(const wxSheetCoords &p_coords)
{
  const gbtGame &game = m_doc->GetGame();

  if (IsRowLabelCell(p_coords)) {
    return m_doc->GetGame()->GetPlayer(m_view->GetRowPlayer())->GetStrategy(p_coords.GetRow() + 1)->GetLabel().c_str();
  }
  else if (IsColLabelCell(p_coords)) {
    return m_doc->GetGame()->GetPlayer(m_view->GetColPlayer())->GetStrategy(p_coords.GetCol() / m_doc->GetGame()->NumPlayers() + 1)->GetLabel().c_str();
  }
  else if (IsCornerLabelCell(p_coords)) {
    return "";
  }

  int rowStrat = p_coords.GetRow();
  int colStrat = p_coords.GetCol() / m_doc->GetGame()->NumPlayers();
  int player = p_coords.GetCol() % m_doc->GetGame()->NumPlayers() + 1;

  gbtGameContingency profile = game->NewContingency();
  for (int pl = 1; pl <= game->NumPlayers(); pl++) {
    profile->SetStrategy(game->GetPlayer(pl)->GetStrategy(m_view->GetStrategy(pl)));
  }

  profile->SetStrategy(game->GetPlayer(m_view->GetRowPlayer())->GetStrategy(rowStrat + 1));
  profile->SetStrategy(game->GetPlayer(m_view->GetColPlayer())->GetStrategy(colStrat + 1));

  return ToText(profile->GetPayoff(game->GetPlayer(player))).c_str();
}

void gbtMatrixSheet::SetCellValue(const wxSheetCoords &p_coords,
			    const wxString &p_value)
{
  if (IsLabelCell(p_coords))  return;

  int rowStrat = p_coords.GetRow();
  int colStrat = p_coords.GetCol() / m_doc->GetGame()->NumPlayers();
  int player = p_coords.GetCol() % m_doc->GetGame()->NumPlayers() + 1;

  const gbtGame &game = m_doc->GetGame();
  gbtGameContingency profile = game->NewContingency();
  for (int pl = 1; pl <= game->NumPlayers(); pl++) {
    profile->SetStrategy(game->GetPlayer(pl)->GetStrategy(m_view->GetStrategy(pl)));
  }

  profile->SetStrategy(game->GetPlayer(m_view->GetRowPlayer())->GetStrategy(rowStrat + 1));
  profile->SetStrategy(game->GetPlayer(m_view->GetColPlayer())->GetStrategy(colStrat + 1));

  gbtGameOutcome outcome = profile->GetOutcome();

  if (outcome.IsNull()) {
    outcome = m_doc->NewOutcome();
    profile->SetOutcome(outcome);
  }

  gbtRational r;
  m_doc->SetPayoff(outcome,
		   m_doc->GetGame()->GetPlayer(player),
		   FromText(p_value.c_str(), r));

  ForceRefresh();
}

wxSheetCellAttr gbtMatrixSheet::GetAttr(const wxSheetCoords &p_coords,
					   wxSheetAttr_Type) const
{
  if (IsRowLabelCell(p_coords)) {
    wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
    attr.SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD));
    attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
    attr.SetOrientation(wxHORIZONTAL);
    attr.SetReadOnly(FALSE);
    attr.SetForegroundColour(s_colors[(m_view->GetRowPlayer() - 1) % 8]);
    return attr;
  }
  else if (IsColLabelCell(p_coords)) {
    wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
    attr.SetFont(wxFont(12, wxSWISS, wxNORMAL, wxBOLD));
    attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
    attr.SetOrientation(wxHORIZONTAL);
    attr.SetReadOnly(FALSE);
    attr.SetForegroundColour(s_colors[(m_view->GetColPlayer() - 1) % 8]);
    return attr;
  }
  else if (IsCornerLabelCell(p_coords)) {
    return GetSheetRefData()->m_defaultGridCellAttr;
  }

  wxSheetCellAttr attr(GetSheetRefData()->m_defaultGridCellAttr);
  attr.SetFont(wxFont(10, wxSWISS, wxNORMAL, wxBOLD));
  attr.SetAlignment(wxALIGN_CENTER, wxALIGN_CENTER);
  attr.SetOrientation(wxHORIZONTAL);
  switch (p_coords.GetCol() % m_doc->GetGame()->NumPlayers()) {
  case 0:
    attr.SetForegroundColour(s_colors[(m_view->GetRowPlayer() - 1) % 8]);
    break;
  case 1:
    attr.SetForegroundColour(s_colors[(m_view->GetColPlayer() - 1) % 8]);
    break;
  default:
    attr.SetForegroundColour(s_colors[(m_view->GetTablePlayer(p_coords.GetCol() % m_doc->GetGame()->NumPlayers() - 1) - 1) % 8]);
    break;
  }
			   
  attr.SetReadOnly(FALSE);
  return attr;
}

void gbtMatrixSheet::OnUpdate(void)
{ 
  BeginBatch();
  int stratRows = GetNumberRows();
  int stratCols = GetNumberCols() / m_doc->GetGame()->NumPlayers();

  const gbtGame &game = m_doc->GetGame();

  int dim = game->GetPlayer(m_view->GetRowPlayer())->NumStrategies();
  if (dim < stratRows) {
    DeleteRows(0, stratRows - dim);
  }
  else if (dim > stratRows) {
    InsertRows(0, dim - stratRows); 
  }

  dim = game->GetPlayer(m_view->GetColPlayer())->NumStrategies();
  if (dim < stratCols) {
    DeleteCols(0, game->NumPlayers() * (stratCols - dim));
  }
  else if (dim > stratCols) {
    InsertCols(0, game->NumPlayers() * (dim - stratCols));
  }

  AutoSizeRows();
  AutoSizeColumns();
  for (int col = 0; col <= GetNumberCols(); col++) {
    if (GetColWidth(col) < GetRowHeight(col)) {
      SetColWidth(col, GetRowHeight(col));
    }
  }
  EndBatch();
  AdjustScrollbars();
}

//-----------------------------------------------------------------------
//                  class gbtTablePlayerCtrl
//-----------------------------------------------------------------------

BEGIN_EVENT_TABLE(gbtTableMatrix, wxPanel)
  EVT_SIZE(gbtTableMatrix::OnSize)
END_EVENT_TABLE()

gbtTableMatrix::gbtTableMatrix(wxWindow *p_parent, 
			       gbtGameDocument *p_doc)
  : wxPanel(p_parent, -1), gbtGameView(p_doc),
    m_sheet(0), m_rowPlayerLabel(0), m_colPlayerLabel(0), m_tableChoiceCtrl(0),
    m_rowPlayer(1), m_colPlayer(2),
    m_contingency(p_doc->GetGame()->NumPlayers())
{
  for (int pl = 1; pl <= m_contingency.Length(); m_contingency[pl++] = 1);

  m_sheet = new gbtMatrixSheet(this);
  m_colPlayerLabel = new gbtPlayerLabelCtrl(this, false);
  m_rowPlayerLabel = new gbtPlayerLabelCtrl(this, true);
  m_tableChoiceCtrl = new gbtTableChoiceCtrl(this);

  RefreshLayout();
}

int gbtTableMatrix::GetTablePlayer(int index) const
{
  int pl = 0;
  while (index > 0) {
    pl++;
    if (pl != GetRowPlayer() && pl != GetColPlayer()) {
      index--;
    }
  }
  return pl;
}

void gbtTableMatrix::SetPlayers(int p_row, int p_col)
{ 
  m_rowPlayer = p_row;
  m_colPlayer = p_col;
  m_sheet->OnUpdate();
  m_rowPlayerLabel->Refresh();
  m_colPlayerLabel->Refresh();
  m_tableChoiceCtrl->Refresh();
}

void gbtTableMatrix::SetStrategy(int pl, int st)
{
  m_contingency[pl] = st;
  m_sheet->OnUpdate();
  m_tableChoiceCtrl->Refresh();
}

void gbtTableMatrix::OnUpdate(void)
{ }

void gbtTableMatrix::OnSize(wxSizeEvent &)
{ RefreshLayout(); }

void gbtTableMatrix::RefreshLayout(void)
{
  if (!m_sheet || !m_rowPlayerLabel || !m_colPlayerLabel) return;

  int width = m_sheet->GetBestSize().GetWidth() + 20;
  if (width > GetClientSize().GetWidth() - 40) {
    width = GetClientSize().GetWidth() - 40;
  }

  int height = m_sheet->GetBestSize().GetHeight() + 20;
  int topMargin = (m_tableChoiceCtrl) ? m_tableChoiceCtrl->GetBestHeight() : 0;
  if (height > GetClientSize().GetHeight() - topMargin) {
    height = GetClientSize().GetHeight() - topMargin;
  }

  if (m_tableChoiceCtrl) {
    m_tableChoiceCtrl->SetSize(40, 0, 
			       GetClientSize().GetWidth() - 80, 
			       m_tableChoiceCtrl->GetBestHeight());

    m_rowPlayerLabel->SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_rowPlayerLabel->SetSize(0, topMargin + 40, 40, height); 

    m_colPlayerLabel->SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_colPlayerLabel->SetSize(40, topMargin, width, 40);

    SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_sheet->SetSize(40, topMargin + 40, 
		     GetClientSize().GetWidth() - 40,
		     GetClientSize().GetHeight() - topMargin - 40);
  }
  else {
    m_rowPlayerLabel->SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_rowPlayerLabel->SetSize(0, 40, 40, height); 

    m_colPlayerLabel->SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_colPlayerLabel->SetSize(40, 0, width, 40);

    SetBackgroundColour(m_sheet->GetCornerLabelAttr().GetBackgroundColour());
    m_sheet->SetSize(40, 40, 
		     GetClientSize().GetWidth() - 40,
		     GetClientSize().GetHeight() - 40);
  }
}
