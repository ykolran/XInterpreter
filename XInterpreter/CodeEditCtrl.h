#pragma once
#include "XToken.h"
#include <unordered_map>

/////////////////////////////////////////////////////////////////////////////
// CCodeEditCtrl window
class CCodeEditCtrl : public CRichEditCtrl
{
// Construction
public:
	enum Types
	{
		TYPE_KEYWORD,
		TYPE_VARIABLE,
		TYPE_STRING,
		TYPE_NUMBER,
		TYPE_FUNCTION,
		TYPE_NATIVE_FUNCTION,
		TYPE_FILE,
		TYPE_COMMENT,
		TYPE_OTHER,
		TYPE_ERROR
	};

	CCodeEditCtrl();
	
// Attributes
public:

// Operations
protected:
	void Autocomplete(BOOL givenext = TRUE);
	CString complete(const CString& str, const CString& actual, CStringArray &matching);
	inline void RemoveListbox();

// Data
	BOOL m_autocompleted;
	CStringArray m_keywords;
	BOOL m_bInForcedChange;

	CListBox m_listbox;

public:
	/* Dictionary */
	void Colorize(int start, unsigned int length, Types type);
	void GetDictionary(CStringArray& dictionary);
	void ResetDictionary();
	BOOL AddKeyword(const CString& str);
	BOOL IsKeyword(const CString& str);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCodeEditCtrl)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
	virtual ~CCodeEditCtrl();

	// Generated message map functions
protected:
	BOOL GetCurWord(CString& curword);
	//{{AFX_MSG(CCodeEditCtrl)
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnListBoxChanged();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	//}}AFX_MSG


	DECLARE_MESSAGE_MAP()
public:
	virtual void PreSubclassWindow();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
