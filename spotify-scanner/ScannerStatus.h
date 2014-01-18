#pragma once



// ScannerStatus form view

class ScannerStatus : public CFormView
{
	DECLARE_DYNCREATE(ScannerStatus)

protected:
	ScannerStatus();           // protected constructor used by dynamic creation
	virtual ~ScannerStatus();

public:
	enum { IDD = IDD_SCANNERSTATUS };
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
};


