#include "stdafx.h"
#include "resource.h"
#include "CodeEditCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCodeEditCtrl

CCodeEditCtrl::CCodeEditCtrl()
{
	m_autocompleted=FALSE;
}

CCodeEditCtrl::~CCodeEditCtrl()
{
}


BEGIN_MESSAGE_MAP(CCodeEditCtrl, CRichEditCtrl)
	//{{AFX_MSG_MAP(CCodeEditCtrl)
	ON_WM_CHAR()
	ON_LBN_SELCHANGE(IDC_LIST, OnListBoxChanged)
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_MOUSEWHEEL()
	ON_WM_KILLFOCUS()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCodeEditCtrl message handlers

CString CCodeEditCtrl::complete(const CString& str, const CString& actual, CStringArray &matching)
{
	int tot=m_keywords.GetSize();
	CString ret;
	int foundindex=-1;
	int comp;
	BOOL found=FALSE;
	int strln = str.GetLength();
	
	int i;
	
	/* find first matching */
	
	int bottom=0, top=tot, middle;
	
	while(1)
	{
		if (bottom >= top)
		{
			i= -1;
			break;
		}
		
		middle = top + bottom;
		middle >>= 1;
		
		comp = strncmp(m_keywords[middle],str, strln);
		
		if (!comp)
		{
			while (middle>=0 && !strncmp(m_keywords[middle],str, strln))
				middle--;
			
			i = middle + 1;
			break;
		};
		
		if (comp < 0)
			bottom=middle+1;
		else
			top=middle;
	}
	
	/* look for next items matching */
	if (i!=-1)
	{
		for (; i<tot; i++)
		{
			if (strncmp(m_keywords[i], str, strln)==0)
			{
				matching.Add(m_keywords[i]);
				
				if (!found)
				{
					ret=m_keywords[i].Mid(strln);
					comp=strcmp(ret,actual);
					if (comp>0)
						found = TRUE;
					
					if (comp<0 && foundindex==-1)
						foundindex=i;
				}
			}
		}
	}
	
	if (found)
		return ret;
	
	if (foundindex!=-1)
		return m_keywords[foundindex].Mid(str.GetLength());
	
	if (matching.GetSize()>1)
		return matching[0];
	
	return "";
}

inline void CCodeEditCtrl::RemoveListbox()
{
	if (m_listbox.m_hWnd)
		m_listbox.DestroyWindow();
	
	m_autocompleted = FALSE;
}

void CCodeEditCtrl::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if (nChar == ' ' && GetKeyState(VK_CONTROL)& 0xE000)
	{
		Autocomplete();
		return;
	}
	else if (m_autocompleted)
	{

		CHARRANGE curSel;
		GetSel(curSel);
		
		if (nChar == VK_BACK)
		{
			CString cw;
			GetCurWord(cw);
			m_autocompleted=FALSE;
			if(!cw.IsEmpty())
			{
				SetSel(curSel.cpMin-1,curSel.cpMax);
				CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
				Autocomplete();
			}
			else
			{
				CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
				RemoveListbox();
			}

		}
		else if (!isalpha(nChar))
		{
			SetSel(curSel.cpMax,curSel.cpMax);

			m_autocompleted=FALSE;
			
			if (m_listbox.m_hWnd)
				m_listbox.DestroyWindow();
			
			CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
		}
		else
		{
			m_autocompleted=FALSE;
			CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
			
			Autocomplete();
		}
	}
	else
	{
		m_autocompleted=FALSE;
		CRichEditCtrl::OnChar(nChar, nRepCnt, nFlags);
	}
}

void CCodeEditCtrl::OnListBoxChanged()
{
	CString temp, curword;
	m_listbox.GetText(m_listbox.GetCurSel(),temp);
	
	GetCurWord(curword);

	CHARRANGE curSel;
	GetSel(curSel);
	temp=temp.Right(temp.GetLength()-curword.GetLength());
	ReplaceSel(temp, TRUE);
	SetSel(curSel.cpMin, curSel.cpMin + temp.GetLength());

	SetFocus();
}

BOOL CCodeEditCtrl::GetCurWord(CString &curword)
{
	CHARRANGE curSel;
	GetSel(curSel);
	
	CString text;
	
	int l=LineFromChar(curSel.cpMin);
	int nLineLength = LineLength(curSel.cpMin);
	char* buffer = new char[nLineLength+4];

	ASSERT(::IsWindow(m_hWnd));
	*(LPWORD)buffer = (WORD)nLineLength;
	nLineLength = ::SendMessage(m_hWnd, EM_GETLINE, l, (LPARAM)buffer);
	
	if (nLineLength>2 && buffer[nLineLength-1]=='\n' && buffer[nLineLength-2]=='\r')
		buffer[nLineLength-2]=0;
	else
		buffer[nLineLength]=0;
	
	int from=curSel.cpMin-LineIndex(l);
	int to=from;
	
	// while (from && buffer[from]!=' ' && buffer[from]!='(' && buffer[from]!='=')
	while (from && buffer[from]!=' ')
		from--;
	
	if (from==-1)
	{
		delete buffer;
		return FALSE;
	}
	
	if (!from && buffer[from]!=' ')
		from--;

	text = buffer+from+1;
	curword = text.Left(to-from-1);

	delete buffer;

	return TRUE;
}

void CCodeEditCtrl::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	/* Check for virtual keys if the listbox is open*/
	if(m_listbox.m_hWnd)
	{
		switch (nChar)
		{
		case (VK_UP):
		case (VK_DOWN):
		case (VK_PRIOR):
		case (VK_NEXT):
		case (VK_HOME):
		case (VK_END):
			{
				m_listbox.SendMessage(WM_KEYDOWN, nChar, MAKELPARAM(nRepCnt, nFlags));
				return;
			}
		case (VK_TAB):
		case (VK_RETURN):
		case (VK_SPACE):
			{
				CHARRANGE curSel;
				GetSel(curSel);
				SetSel(curSel.cpMax,curSel.cpMax);
				
				RemoveListbox();
				
				return;
			} 
		case (VK_RIGHT):
			{
				CHARRANGE curSel;
				GetSel(curSel);
				SetSel(curSel.cpMin+1, curSel.cpMax);
				
				Autocomplete( FALSE );
				
				return;
			}
		case (VK_LEFT):
			{
				CString cword;
				GetCurWord(cword);
				
				if (!cword.IsEmpty())
				{
					CHARRANGE curSel;
					GetSel(curSel);
					SetSel(curSel.cpMin-1, curSel.cpMax);
					
					Autocomplete( FALSE );
				}
				else
				{
					ReplaceSel("");
					RemoveListbox();
				}
				return;
			}
		case (VK_ESCAPE):
			{
				RemoveListbox();
				
				ReplaceSel("");
				
				return;
			}
		}
	}
	
	switch (nChar)
	{
	case (VK_TAB):
		ReplaceSel("\t", TRUE);
		return;
	}
	CRichEditCtrl::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CCodeEditCtrl::Autocomplete(BOOL givenext)
{
	CHARRANGE curSel;
	CString text, act, rep;
	CStringArray strings;
	
	if(!GetCurWord(text))
		return;
	
	GetSel(curSel);
	
	if (curSel.cpMax != curSel.cpMin)
		act = GetSelText();
	
	rep=complete(text, act, strings);
	if (!givenext)
		rep= act;

	m_autocompleted=TRUE;

	if (strings.GetSize() != 0)
	{
		ReplaceSel(rep, TRUE);
		SetSel(curSel.cpMin, curSel.cpMin + rep.GetLength());

		CRect p;
		CHARFORMAT cf;
		GetSelectionCharFormat(cf);

		p.top = GetCharPos(curSel.cpMin - text.GetLength()).y + cf.yHeight / 10;
		p.left = GetCharPos(curSel.cpMin - text.GetLength()).x;
		p.right = p.left + 100;

		if (!m_listbox.m_hWnd)
		{
			m_listbox.Create(CBS_DROPDOWNLIST | WS_VSCROLL | WS_CHILD | WS_VISIBLE | LBS_STANDARD | LBS_HASSTRINGS, p, this, IDC_LIST);
			m_listbox.SetFont(GetParent()->GetFont());
		}
		else
		{
			m_listbox.ResetContent();
		}

		p.bottom = p.top + m_listbox.GetItemHeight(0) * min(strings.GetSize() + 1, 10);	// The cbox must not be too long
		m_listbox.MoveWindow(&p);

		for (int i = 0; i < strings.GetSize(); i++)
			m_listbox.AddString(strings[i]);
		
		m_listbox.SelectString(-1, text + rep);
	}
	else
	{
		if (!m_listbox.m_hWnd)
			return;

		m_listbox.SetCurSel(-1);
		
		/*looking for nearest*/
		int nofitems=m_listbox.GetCount();
		CString temp;

		int i = 0;
		for (; i < nofitems; i++)
		{
			m_listbox.GetText(i,temp);
			if (temp.CompareNoCase(text)>0)
				break;
		}

		m_listbox.SetTopIndex(i--);
	}
		
	return;
}

/************************************************************/
/*				Forwarding some messages					*/
/************************************************************/

BOOL CCodeEditCtrl::PreTranslateMessage(MSG* pMsg) 
{
	if (pMsg->hwnd==m_listbox.m_hWnd && pMsg->message==WM_CHAR)
		SetFocus();
	
	return CRichEditCtrl::PreTranslateMessage(pMsg);
}

BOOL CCodeEditCtrl::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
	if (m_listbox.m_hWnd)
		return m_listbox.SendMessage(WM_MOUSEWHEEL, MAKEWPARAM(nFlags, zDelta), MAKELPARAM( pt.x, pt.y));
	
	return CRichEditCtrl::OnMouseWheel(nFlags, zDelta, pt);
}

/************************************************************/
/*			Some minor overloaded funcions					*/
/************************************************************/

void CCodeEditCtrl::OnKillFocus(CWnd* pNewWnd) 
{
	RemoveListbox();
	CRichEditCtrl::OnKillFocus(pNewWnd);
}

void CCodeEditCtrl::OnLButtonDown(UINT nFlags, CPoint point) 
{
	RemoveListbox();
	CRichEditCtrl::OnLButtonDown(nFlags, point);
}

void CCodeEditCtrl::OnRButtonDown(UINT nFlags, CPoint point) 
{
	RemoveListbox();
	CRichEditCtrl::OnRButtonDown(nFlags, point);
}

void CCodeEditCtrl::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	CRichEditCtrl::OnLButtonDblClk(nFlags, point);
}

/************************************************************/
/*						Dictionary							*/
/************************************************************/

void CCodeEditCtrl::ResetDictionary()
{
	m_keywords.RemoveAll();
}


void CCodeEditCtrl::GetDictionary(CStringArray &dictionary)
{
	dictionary.RemoveAll();
	dictionary.Copy(m_keywords);
}

BOOL CCodeEditCtrl::AddKeyword(const CString& str)
{
	if (str.IsEmpty())
		return FALSE;

	if (!IsKeyword(str))
	{
		int i, tot = m_keywords.GetSize();
		
		for (i = 0 ;i < tot; i++)
		{
			if (strcmp(m_keywords[i],str)>0)
				break;
		}

		m_keywords.InsertAt(i,str);
		return TRUE;
	}
	return FALSE;
}

BOOL CCodeEditCtrl::IsKeyword(const CString& str)
{
	int bottom=0, top=m_keywords.GetSize(), middle, comp;

		while(1)
		{
			if (bottom >= top)
				return FALSE;

			middle = top + bottom;
			middle >>= 1;

			comp=strcmp(m_keywords[middle],str);

			if (!comp)
				return TRUE;
		
			if (comp < 0)
				bottom=middle+1;
			else
				top=middle;
		}

}

void CCodeEditCtrl::PreSubclassWindow()
{
	PARAFORMAT pf;
	pf.cbSize = sizeof(PARAFORMAT);
	pf.dwMask = PFM_TABSTOPS;
	pf.cTabCount = MAX_TAB_STOPS;
	for (int itab = 0; itab < pf.cTabCount; itab++)
		pf.rgxTabs[itab] = (itab + 1) * 480;
	SetParaFormat(pf);

	SetEventMask(GetEventMask() | ENM_CHANGE);

	CHARFORMAT format;
	format.dwMask = CFM_CHARSET | CFM_FACE | CFM_SIZE | CFM_OFFSET | CFM_COLOR;
	format.dwMask ^= CFM_ITALIC ^ CFM_BOLD ^ CFM_STRIKEOUT ^ CFM_UNDERLINE;
	format.dwEffects = CFE_BOLD;
	format.yHeight = 200; //10pts * 20 twips/point = 200 twips
	format.bCharSet = ANSI_CHARSET;
	format.bPitchAndFamily = FIXED_PITCH | FF_MODERN;
	format.yOffset = 0;
	strcpy_s(format.szFaceName, "Courier New");
	format.cbSize = sizeof(format);
	format.crTextColor = RGB(0, 0, 0);

	SetDefaultCharFormat(format);

	CRichEditCtrl::PreSubclassWindow();
}
