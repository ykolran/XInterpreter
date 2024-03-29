
// XInterpreterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "XInterpreter.h"
#include "XInterpreterDlg.h"
#include "afxdialogex.h"
#include "XChunk.h"
#include "XVM.h"
#include "XToken.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CXInterpreterDlg dialog



CXInterpreterDlg::CXInterpreterDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_XINTERPRETER_DIALOG, pParent)
	, m_source(_T(""))
	, m_output(_T(""))
	, m_executionTrace(_T(""))
	, m_byteCode(_T(""))
	, m_bTrace(FALSE)
	, m_bByteCode(FALSE)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_cEdit1.AddKeyword("var");
	m_cEdit1.AddKeyword("using");
	m_cEdit1.AddKeyword("if");
	m_cEdit1.AddKeyword("while");
	m_cEdit1.AddKeyword("for");
	m_cEdit1.AddKeyword("or");
	m_cEdit1.AddKeyword("and");
	m_cEdit1.AddKeyword("true");
	m_cEdit1.AddKeyword("false");
	m_cEdit1.AddKeyword("this");
	m_cEdit1.AddKeyword("this");
	m_cEdit1.AddKeyword("print");


	m_files.push_back(std::make_pair("Ref", new CLogDataFile(4, 100)));
	m_files[0].second->columns = std::vector<std::string>{ "Time", "X", "Y", "Z" };
	for (int i = 0; i < 100; i++)
	{
		m_files[0].second->m_data[0][i] = 0.1*i + 10;
		m_files[0].second->m_data[1][i] = 6400000 + i;
		m_files[0].second->m_data[2][i] = i;
		m_files[0].second->m_data[3][i] = 100+i;
	}

	m_files.push_back(std::make_pair("LS", new CLogDataFile(7, 50)));
	m_files[1].second->columns = std::vector<std::string>{ "Time", "X", "Y", "Z", "VX", "VY", "VZ" };
	for (int i = 0; i < 50; i++)
	{
		m_files[1].second->m_data[0][i] = 0.2*i;
		m_files[1].second->m_data[1][i] = 6400000 + 2*i + 5;
		m_files[1].second->m_data[2][i] = 2*i + 5;
		m_files[1].second->m_data[3][i] = 100 + 2*i + 5;
		m_files[1].second->m_data[4][i] = 10;
		m_files[1].second->m_data[5][i] = 10;
		m_files[1].second->m_data[6][i] = 10;
	}
}

CXInterpreterDlg::~CXInterpreterDlg()
{
	for (auto file : m_files)
		delete file.second;
}

void CXInterpreterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT1, m_cEdit1);
	DDX_Text(pDX, IDC_EDIT4, m_output);
	DDX_Text(pDX, IDC_EDIT3, m_executionTrace);
	DDX_Text(pDX, IDC_EDIT2, m_byteCode);
	DDX_Check(pDX, IDC_CHECK_EXECUTION_TRACE, m_bTrace);
	DDX_Check(pDX, IDC_CHECK_BYTE_CODE, m_bByteCode);
}

BEGIN_MESSAGE_MAP(CXInterpreterDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_EN_CHANGE(IDC_EDIT1, &CXInterpreterDlg::OnEnChangeEdit1)
	ON_BN_CLICKED(IDC_CHECK_EXECUTION_TRACE, &CXInterpreterDlg::OnBnClickedCheckExecutionTrace)
	ON_BN_CLICKED(IDC_CHECK_BYTE_CODE, &CXInterpreterDlg::OnBnClickedCheckByteCode)
END_MESSAGE_MAP()


// CXInterpreterDlg message handlers

BOOL CXInterpreterDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CXInterpreterDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CXInterpreterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CXInterpreterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CXInterpreterDlg::error(int line, std::string message) {
	report(line, "", message);
}

void CXInterpreterDlg::report(int line, std::string where, std::string message) {
	CString serr;
	serr.Format("[line %d] Error %s: %s\r\n", line, where.c_str(), message.c_str());
	m_output += serr;
	UpdateData(FALSE);

	m_hadError = true;
}

void CXInterpreterDlg::OnEnChangeEdit1()
{
	UpdateData(TRUE);
	
	m_output = "";
	m_executionTrace = "";
	m_byteCode = "";
	m_hadError = false;
	m_hadRuntimeError = false;

	if (m_cEdit1.GetTextLength() > 0)
	{
		m_cEdit1.GetTextRange(0, m_cEdit1.GetTextLength(), m_source);

		XVM xvm;
		xvm.interpret((LPCSTR)m_source, m_bTrace, m_bByteCode);
	}
	UpdateData(FALSE);
}




void CXInterpreterDlg::OnBnClickedCheckExecutionTrace()
{
	OnEnChangeEdit1();
}


void CXInterpreterDlg::OnBnClickedCheckByteCode()
{
	OnEnChangeEdit1();
}
