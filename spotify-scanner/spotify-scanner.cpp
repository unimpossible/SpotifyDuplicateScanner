// spotify-scanner.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "spotify-scanner.h"
#include "scanner-events.h"
#include "keys.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;								// current instance
TCHAR szTitle[MAX_LOADSTRING];					// The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];			// the main window class name
HWND m_hWnd;									// Handle to main window accessible from other threads.

//Global Spotify Variables
log4cplus::Logger logger;
std::string username;
std::string password;
bool g_bRemoveDuplicates = TRUE;
bool g_bRemember = TRUE;

//Controls thread schedule.
boost::threadpool::pool thread_pool(1);

//Main thread
DWORD m_mainThread;
//Sent through threads involving sp session
boost::mutex mtx_done;
boost::mutex mtx_scanning;
bool done = false;
bool g_bFirstRunComplete = false;
boost::shared_ptr<spotify::Session> session;
boost::shared_ptr<spotify::PlayListContainer> plContainer;

boost::thread thread_event_scheduler;
boost::thread update_scheduler;

//Global Duplicate List
std::list<TrackEntry> duplicateList;


using namespace log4cplus;

// Forward declarations of functions included in this code module:
ATOM				MyRegisterClass(HINSTANCE hInstance);
BOOL				InitInstance(HINSTANCE, int);
LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	About(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK	LogCallback(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK LoginCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);

int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPTSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

 	// TODO: Place code here.
	MSG msg;
	HACCEL hAccelTable;

	//Get main thread ID and save.
	m_mainThread = GetCurrentThreadId();

	// Initialize Logger
	boost::thread initLoggingThread([&]{
		scanner_events_initLogging();
	});

	initLoggingThread.join();

	//Initialize session object and threading
	boost::thread initSessionThread([&]{
		scanner_events_initSession();
	});

//	boost::shared_ptr<spotify::Session> orig_session;(new spotify::Session());
//	boost::shared_ptr<scanner_session> session = boost::dynamic_pointer_cast<scanner_session>(orig_session);

	
	// Initialize global strings
	LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadString(hInstance, IDC_SPOTIFYSCANNER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// Perform application initialization:
	if (!InitInstance (hInstance, nCmdShow))
	{
		return FALSE;
	}

	//After instance is initiated, attempt to Relogin an old session.
	boost::thread tryLoginRemembered([&]{
		scanner_events_try_remembered_login();
	});

	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SPOTIFYSCANNER));

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	
	return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SPOTIFYSCANNER));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDC_SPOTIFYSCANNER);
	wcex.lpszClassName	= szWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

	return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   HWND hWnd;
   

   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);
  
   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);
   m_hWnd = hWnd; //Set global pointer for use in message sending!

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int wmId, wmEvent;
	PAINTSTRUCT ps;
	HDC hdc;

	switch (message)
	{
	case WM_COMMAND:
		wmId    = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		// Parse the menu selections:
		switch (wmId)
		{
		case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
			break;
		case ID_SPOTIFY_LOGIN:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, LoginCallback);
			break;
		case ID_SPOTIFY_LOGOUT:
			scanner_events_logout();
			break;
		case ID_SPOTIFY_RESCAN:
			boost::thread([&]{
				scanner_events_duplicates_runScan();
			});
			break;
		case ID_SPOTIFY_DELETEDUPLICATES:
			{
				//Change setting to remove duplicates or not.
				
				//Reset menu item
				HMENU menu = GetMenu(hWnd);
				if(g_bRemoveDuplicates)
					CheckMenuItem(menu, ID_SPOTIFY_DELETEDUPLICATES, MF_UNCHECKED);
				else
					CheckMenuItem(menu, ID_SPOTIFY_DELETEDUPLICATES, MF_CHECKED);

				g_bRemoveDuplicates = !g_bRemoveDuplicates;
				break;
			}
		case IDM_EXIT:
		{
			//Cleanup and destroy window.
			scanner_exit_cleanup();
			//PostQuiteMessage occurs now after cleanup method returns.
			//PostQuitMessage(0);
		}
			break;
		case ID_FILE_LOG:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_LOG), hWnd, LogCallback);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
		break;
	case UWM_SCANNER_MSG_POST_TO_DIALOG:{
		//MessageBox(hWnd, (LPCTSTR)lParam, "Message", MB_OK);
		RECT windowRect = {0};
		GetWindowRect(hWnd, &windowRect);
		hdc = GetDC(hWnd);
		FillRect(hdc, &windowRect, (HBRUSH)0xFF);
		DrawText(hdc, (LPCSTR)lParam, -1, &windowRect, DT_EDITCONTROL);
		ReleaseDC(hWnd, hdc);
		break;
		}
	//A callback returning upon login
	case UWM_SCANNER_LOGGED_IN:
	{
		boost::thread([&]{
			scanner_events_duplicates_startScan();
		});

		hdc = GetDC(hWnd);
		TextOut(hdc, 0, 0, "Logged In!", 10);
		ReleaseDC(hWnd, hdc);

		//Do not allow scanning or logout until scan finished.
		HMENU menu = GetMenu(hWnd);
		EnableMenuItem(menu, ID_SPOTIFY_LOGOUT, MF_GRAYED);
		EnableMenuItem(menu, ID_SPOTIFY_LOGIN, MF_GRAYED);
		EnableMenuItem(menu, ID_SPOTIFY_RESCAN, MF_ENABLED);
		break;
	}
	case UWM_SCANNER_LOGIN_FAIL:
	{
		MessageBox(hWnd, "Spotify login failed!", "Error", MB_OK);
		DialogBox(hInst, MAKEINTRESOURCE(IDD_LOGIN), hWnd, LoginCallback);
		break;
	}
	//When scan is started we will disable menu items.
	case UWM_SCANNER_SCAN_STARTED:
	{
		HMENU menu = GetMenu(hWnd);
		EnableMenuItem(menu, ID_SPOTIFY_LOGIN, MF_GRAYED);
		EnableMenuItem(menu, ID_SPOTIFY_LOGOUT, MF_GRAYED);
		EnableMenuItem(menu, ID_SPOTIFY_RESCAN, MF_GRAYED);
		break;
	}
	//Message upon finished scan. Re-enable menu items.
	case UWM_SCANNER_SCAN_FINISHED:
	{
		HMENU menu = GetMenu(hWnd);
		EnableMenuItem(menu, ID_SPOTIFY_LOGOUT, MF_ENABLED);
		EnableMenuItem(menu, ID_SPOTIFY_RESCAN, MF_ENABLED);
		//Allow tracks_added and playlist_added to change this value
		g_bFirstRunComplete = true;
		break;
	}
	//Message which is sent from spotify session upon new tracks or a new playlist
	case UWM_SCANNER_SCAN_REQUESTED:
	{
		//Because we have setup a mutex which requires lock on start of scan, we will not scan
		//Until other scans have completed.
		boost::thread([&]{
				scanner_events_duplicates_runScan();
		});
		break;
	}
		
	//Returns message upon logout
	case UWM_SCANNER_LOGGED_OUT:
	{
		hdc = GetDC(hWnd);
		TextOut(hdc, 0, 0, "Logged out.", 10);
		ReleaseDC(hWnd, hdc);
		//Allow login button again
		HMENU menu = GetMenu(hWnd);
		EnableMenuItem(menu, ID_SPOTIFY_LOGOUT, MF_GRAYED);
		EnableMenuItem(menu, ID_SPOTIFY_LOGIN, MF_ENABLED);
		break;
	}
	//Returned after cleanup is finished
	case UWM_SCANNER_LOGGED_OUT_CLEANED_UP:
		{
			PostQuitMessage(0);
		}
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		// TODO: Add any drawing code here...
		EndPaint(hWnd, &ps);
		break;
	case WM_DESTROY:
		scanner_exit_cleanup();
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

// Message handler for Log Dialog box.
INT_PTR CALLBACK LogCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::string readBuffer;
	static HWND logEdit = NULL;
	

	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		{
		/*hFile = CreateFile("spotify-scanner.log", GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);*/
		logEdit = GetDlgItem(hDlg, IDC_LOGEDIT);
		std::ifstream in("spotifyScanner.log", std::ios::in);;

		if(in){
			if(logEdit == NULL){
				EndDialog(hDlg, 0);
				DestroyWindow(hDlg);
				return (INT_PTR)TRUE;
			}
			in.seekg(0, std::ios::end);
			readBuffer.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&readBuffer[0], readBuffer.size());
			in.close();
			
		}
		else
			readBuffer = "Unable to open file";
		
		SetWindowText(logEdit, _T(readBuffer.c_str()));

		return (INT_PTR)TRUE;
		}
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		if(LOWORD(wParam) == IDREFRESH)
		{
			logEdit = GetDlgItem(hDlg, IDC_LOGEDIT);
			std::ifstream in("spotifyScanner.log", std::ios::in);;

			if(in){
				if(logEdit == NULL){
					EndDialog(hDlg, 0);
					DestroyWindow(hDlg);
					return (INT_PTR)TRUE;
				}
				in.seekg(0, std::ios::end);
				readBuffer.resize(in.tellg());
				in.seekg(0, std::ios::beg);
				in.read(&readBuffer[0], readBuffer.size());
				in.close();
			
			}
			else
				readBuffer = "Unable to open file";
		
			SetWindowText(logEdit, _T(readBuffer.c_str()));

			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}

//Message handler for login
INT_PTR CALLBACK LoginCallback(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hDlg, IDC_REMEMBERME, g_bRemember);
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if ( LOWORD(wParam) == IDOK )
		{
			HWND hUsername = GetDlgItem(hDlg, IDC_USERNAME);
			GetWindowText(hUsername, (LPSTR)username.c_str(), 256);
			HWND hPassword = GetDlgItem(hDlg, IDC_PASSWORD);
			GetWindowText(hPassword, (LPSTR)password.c_str(), 256);
			if(password.c_str() == "" || username.c_str() == "")
			{
				MessageBox(hDlg, "Username or password cannot be blank", "Error", MB_OK);
				return (INT_PTR)TRUE;
			}

			//Check if remember me was selected
			if(IsDlgButtonChecked(hDlg, IDC_REMEMBERME) == BST_CHECKED)
			{
				g_bRemember = TRUE;
			}
			else
			{
				g_bRemember = FALSE;
			}

			//Attempt login with already init session
			boost::thread initSessionThread([&]{
				scanner_events_login();
			});
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		else {
			if( LOWORD(wParam) == IDCANCEL ){
				EndDialog(hDlg, LOWORD(wParam));
				return (INT_PTR)TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}

/**
* 
* Scanner cleanup method.
* Shuts down thread pool and update_scheduler thread
* which is responsible for calling next spotify events
*/
void scanner_exit_cleanup(){
	//We need this method to run in another thread so the logout message can post back
	//to the window and then this method can post the message that all has cleaned up
	//and is safe to quit.
	boost::thread cleanupThread([&] {
		boost::thread logoutThread([&]{
			scanner_events_logout();
		});
		//wait for logout function to return
		logoutThread.join();

		//Shutodnw update scheduler loop inside update_scheduler thread.
		mtx_done.lock();
		done = true;
		mtx_done.unlock();
	
		//stop the update scheduler
		update_scheduler.join();
		//Wait for other misc tasks to finish.
		thread_pool.wait();

		//Send message to kill window and application
		SendMessage(m_hWnd, UWM_SCANNER_LOGGED_OUT_CLEANED_UP, 0, 0);
	});

	cleanupThread.detach(); //explicitly detaching thread
}