
// XInterpreterDlg.h : header file
//

#pragma once
#include "CLogDataFile.h"
#include <vector>
#include <string>
#include <unordered_map>
#include "XToken.h"

// CXInterpreterDlg dialog
class CXInterpreterDlg : public CDialogEx
{
// Construction
public:
	CXInterpreterDlg(CWnd* pParent = nullptr);	// standard constructor
	~CXInterpreterDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_XINTERPRETER_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnEnChangeEdit1();
	void error(int line, std::string message);
	void report(int line, std::string where, std::string message);
	void ColorizeToken(const XTokenData& token);
	bool m_hadError = false; 
	bool m_hadRuntimeError = false;
	CString m_source;
	CRichEditCtrl m_cEdit1;
	CString m_output;
	CString m_byteCode;
	CString m_executionTrace;
	CFont m_font;
	std::vector<std::pair<std::string, CLogDataFile*>> m_files;
	std::unordered_map<int, std::pair<unsigned int, XToken>> m_colorized;
};

inline CXInterpreterDlg* DLG() { return (CXInterpreterDlg*)AfxGetApp()->m_pMainWnd; }