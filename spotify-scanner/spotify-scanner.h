#ifndef SPOTIFY_SCANNER_H
#define SPOTIFY_SCANNER_H


#include "resource.h"

#include <log4cplus/loggingmacros.h>
#include <log4cplus/logger.h>
#include <log4cplus/configurator.h>
#include <log4cplus/fileappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/loglevel.h>


#ifdef WIN32
#include "stdafx.h"
#endif

#include <string>
#include <iostream>
#include <map>

//Fix weird boost bug
#ifndef TIME_UTC
#define TIME_UTC TIME_UTC_
#endif
//Includes from boost for spotify code
#include <boost/thread.hpp>
#include <boost/threadpool.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/format.hpp>



//#include <spotify/Session.hpp>
#include "spotify\Session.hpp"
#include "spotify\PlayListContainer.hpp"
#include "spotify\PlayList.hpp"
#include "spotify\Track.hpp"

/*
 * Logging
 */
extern log4cplus::Logger logger;
#define message(M, ...) LOG4CPLUS_INFO_FMT(logger, LOG4CPLUS_TEXT(M), ##__VA_ARGS__)
#define error(M, ...) LOG4CPLUS_ERROR_FMT(logger, LOG4CPLUS_TEXT(M), ##__VA_ARGS__)


//Spotify Shared Variables
//Following session example. Adding externs and mutex for additional threading.
extern boost::threadpool::pool thread_pool;
extern boost::thread update_scheduler;
extern boost::thread thread_event_scheduler;
extern boost::mutex mtx_done;
extern boost::mutex mtx_scanning;
extern bool done;
extern bool g_bFirstRunComplete;
extern bool g_bRemoveDuplicates;
extern bool g_bRemember;

//Thread var
#ifdef WIN32
extern DWORD m_mainThread;
extern HWND m_hWnd;
#define BUFFERSIZE 81
#else
extern int m_mainThread;
#endif

//Windows Specific Messaging calls:
#ifdef WIN32
#define SCANNER_QUIT_MSG PostThreadMessage(m_mainThread, WM_QUIT, 0, 0)
#define UWM_SCANNER_MSG_POST_TO_DIALOG (WM_APP + 0)
#define UWM_SCANNER_MSG_POST_TO_DIALOG_INFO (WM_APP + 1)

#define UWM_SCANNER_LOGGED_IN (WM_APP + 2)
#define UWM_SCANNER_LOGIN_FAIL (WM_APP + 3)
#define UWM_SCANNER_LOGGED_OUT (WM_APP + 4)
#define UWM_SCANNER_LOGGED_OUT_CLEANED_UP (WM_APP +5)

#define UWM_SCANNER_SCAN_STARTED (WM_APP + 6)
#define UWM_SCANNER_SCAN_FINISHED (WM_APP + 7)
#define UWM_SCANNER_SCAN_REQUESTED (WM_APP + 8)




#define message_dialog(MESSAGESEND)	\
	do{ if(m_hWnd)						\
		SendMessage(m_hWnd, UWM_SCANNER_MSG_POST_TO_DIALOG, 0, (LPARAM)(LPCTSTR)(MESSAGESEND)); }while(0)
#endif //WIN32

extern boost::shared_ptr<spotify::Session> session;
extern  boost::shared_ptr<spotify::PlayListContainer> plContainer;

void scanner_exit_cleanup();

class Scanner_PlayListContainer : public spotify::PlayListContainer {
public:
	 Scanner_PlayListContainer(boost::shared_ptr<spotify::Session> session) : PlayListContainer(session){};
	virtual ~Scanner_PlayListContainer(){};

	void OnContainerLoaded() override;
};

#endif