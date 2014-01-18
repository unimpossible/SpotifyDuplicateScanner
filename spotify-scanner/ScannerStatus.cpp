// ScannerStatus.cpp : implementation file
//

#include "stdafx.h"
#include "spotify-scanner.h"
#include "ScannerStatus.h"


// ScannerStatus

IMPLEMENT_DYNCREATE(ScannerStatus, CFormView)

ScannerStatus::ScannerStatus()
	: CFormView(ScannerStatus::IDD)
{

}

ScannerStatus::~ScannerStatus()
{
}

void ScannerStatus::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ScannerStatus, CFormView)
END_MESSAGE_MAP()


// ScannerStatus diagnostics

#ifdef _DEBUG
void ScannerStatus::AssertValid() const
{
	CFormView::AssertValid();
}

#ifndef _WIN32_WCE
void ScannerStatus::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}
#endif
#endif //_DEBUG


// ScannerStatus message handlers
