#include "scanner-events.h"
#include "keys.h"
#include <boost/thread/lock_types.hpp>

using namespace log4cplus;

void scanner_events_initLogging(){
	BasicConfigurator config;		
	config.configure();
	logger = Logger::getInstance(LOG4CPLUS_TEXT("spotifyScanner"));	
	SharedAppenderPtr logAppender(new FileAppender(LOG4CPLUS_TEXT("spotifyScanner.log")));
	logAppender->setName(LOG4CPLUS_TEXT("spotScanAppender"));
	std::auto_ptr<Layout> logLayout = std::auto_ptr<Layout>(new log4cplus::SimpleLayout());
	logAppender->setLayout(logLayout);
	logger.addAppender(logAppender);
}

//Function used by the update scheduler.
boost::mutex next_timeout_mutex;
int next_timeout = 0;
boost::mutex notify_mutex;
bool notify = false;
boost::condition_variable condition;

boost::function<void ()> reeschedule = [&] {
	boost::lock_guard<boost::mutex> lock(notify_mutex);
	{
		boost::lock_guard<boost::mutex> timeout_lock(next_timeout_mutex);
		next_timeout = session->Update();
	}
	notify = true;
	condition.notify_all();
};
////

/*
*	Runs in separate thread and stays active.
*/
void scanner_events_spawn_UpdateScheduler(){
		session->connectToOnNotifyMainThread([&] {
			thread_pool.schedule(reeschedule);
		});
		
		

		boost::thread update_scheduler([&]{
			//On shutdown, we will be setting the done variable
			//to true to stop this loop. It is done from the main thread.
			mtx_done.lock();
			while (!done) {
				mtx_done.unlock();
				boost::unique_lock<boost::mutex> lock(notify_mutex);
				while (!notify) {
					if (!condition.timed_wait(lock, boost::posix_time::millisec(next_timeout))) {
						// timeout, need to schedule update again
						thread_pool.schedule(reeschedule);
						// we use reeschedule, because we can reenter the while
						// with the old timeout
						notify = false;
					}
				}
				// update was called by someone else need to update timeout
				notify = false;

				//Allow done to change
				mtx_done.lock();
			}
		});

		//Keep alive to process events.
		update_scheduler.detach();
}


/*
	Functions to allow custom callbacks.

	These functions define the callback structures 
	used for certain functions of the scanning process.
*/
sp_session_callbacks getSessionCallbacks(){
	sp_session_callbacks sess_callbacks = {0};
	sess_callbacks.connection_error = &scanner_on_connection_error;
	sess_callbacks.log_message = &scanner_on_log_message;
	sess_callbacks.logged_in = &scanner_callback_logged_in;

	return sess_callbacks;
}

//Each playlist gets this structure
sp_playlist_callbacks getPlaylistCallbacks(){
	sp_playlist_callbacks pl_callbacks = {0};
	pl_callbacks.tracks_added = &scanner_callback_playlist_tracks_added;
	pl_callbacks.tracks_removed = &scanner_callbacks_playlist_tracks_removed;
	pl_callbacks.tracks_moved = &scanner_callbacks_playlist_tracks_moved;

	return pl_callbacks;
}


sp_error scanner_events_initSession()
{
	sp_error error = SP_ERROR_OK;
	boost::mutex mtx_; //mutex will be used on fail conditions
	bool fail = false;
	

	// Attempt to initialize session.
	thread_pool.schedule([&]
	{
		session = spotify::Session::Create();
	});

	while(!session){
		boost::this_thread::sleep(boost::posix_time::millisec(20));
	}

	//User defined callbacks
	sp_session_callbacks sess_callbacks = getSessionCallbacks();
	bool finished = false;
		thread_pool.schedule([&]{
			if(session->Initialise(SpotifyBasicFixture().configuration, sess_callbacks) != SP_ERROR_OK)
			{
				fail = true;
				finished = true;
			}
			else
			{
				finished = true;
			}
		});

		while(!finished){
			boost::this_thread::sleep(boost::posix_time::millisec(20));
		}

		if(fail){
			error("Could not inititialize session!");
			error = SP_ERROR_API_INITIALIZATION_FAILED;
			return error;
		}
		mtx_.unlock();

	return error;
}

//This function runs on startup to determine if we have a saved session:
void scanner_events_try_remembered_login(){
	boost::mutex reloginMutex;
	bool forceWait = true;
	bool reLoggedIn;
	
	thread_pool.schedule([&]{
		bool canRelogin = session->Relogin();

		reloginMutex.lock();
		if(canRelogin)
			reLoggedIn = true; //must send message from other thread, not pool.
		else
			reLoggedIn = false;
		forceWait = false;
		reloginMutex.unlock();
	});

	//Start updateScheduler.
	scanner_events_spawn_UpdateScheduler();

	reloginMutex.lock();
	while(forceWait)
	{
		boost::this_thread::sleep(boost::posix_time::millisec(20));
		reloginMutex.unlock();
	}
	reloginMutex.unlock();

	//Done in regular log method to allow other tasks to occur
	thread_pool.wait();

	/*if(reLoggedIn)
		SendMessage(m_hWnd, UWM_SCANNER_LOGGED_IN, 0, 0);
	else
		SendMessage(m_hWnd, UWM_SCANNER_LOGGED_OUT, 0, 0);*/
	//We are going to wait for the callback to fire on its own..
}

void scanner_events_login(){
	thread_pool.schedule([&]{
		session->Login(username.c_str(), password.c_str(), g_bRemember);
	});
	
	scanner_events_spawn_UpdateScheduler();
	
    while (!session->IsLoggedIn()){
         boost::this_thread::sleep(boost::posix_time::millisec(20));
	}

	thread_pool.wait();
	message("Log in attempt completed.");
	message_dialog("Login Attempt Completed.");
}

///
// Logout and send message back to main window that logged out has occured.
//
sp_error scanner_events_logout()
{
	sp_error error = SP_ERROR_OK;
	thread_pool.schedule([&]{
		session->Logout();
	});

	//Wait for logout to complete.
	while(session->IsLoggedIn())
		boost::this_thread::sleep(boost::posix_time::millisec(20));

	SendMessage(m_hWnd, UWM_SCANNER_LOGGED_OUT, 0, 0);

	return error;
}
void scanner_events_fail()
{
	//cannot exit from other thread
	done = true;
	message("Quittting Spotify-Scanner.");
	SCANNER_QUIT_MSG;
}

bool scanner_remove_old_data(){
	return true;
}

void SP_CALLCONV scanner_on_connection_error(sp_session *session, sp_error error) {
    error("Session::OnConnectionError: %s", sp_error_message(error));
}

void SP_CALLCONV scanner_on_log_message(sp_session *session, const char *data){
	message("Spotify: %s", data);
}
void SP_CALLCONV scanner_callback_logged_in(sp_session *session, sp_error error){
	message("Logged in");
	if(error != SP_ERROR_OK)
	{
		error("Error in logging in: %s", sp_error_message(error));
		SendMessage(m_hWnd, UWM_SCANNER_LOGIN_FAIL, 0, 0);
	}
	else{
		SendMessage(m_hWnd, UWM_SCANNER_LOGGED_IN, 0, 0);
	}
}

/*
* Check duplicates. Start adding necessary callbacks.
*/
void scanner_events_duplicates_startScan(){
	mtx_scanning.lock(); //only one scan allowed at a time..
	if(!scanner_remove_old_data())
	{
		error("Unable to delete old data");
		scanner_events_fail();
	}
	message("Entering scanner_events_duplicates_startScan");
	//Disable login/logout buttons
	SendMessage(m_hWnd, UWM_SCANNER_SCAN_STARTED, 0, 0);

	thread_pool.schedule([&]{
		sp_playlistcontainer_callbacks plContainerCallbacks = {0};
		
		plContainerCallbacks.container_loaded = scanner_callback_container_loaded;
		plContainer = session->GetPlayListContainer(
			&plContainerCallbacks);
	});

	while(!plContainer || plContainer->IsLoading(true))
		boost::this_thread::sleep(boost::posix_time::millisec(300));

	
	message_dialog("Container load finished from loop..");
	scanner_events_duplicates_runScan();
	
	thread_pool.wait();
}

// Run check duplicates on plContainer
void scanner_events_duplicates_runScan(){
	//Reset our duplicate list on each scan
	duplicateList.clear();

	for(int i = 1; i < plContainer->GetNumChildren(); i++)
	{
		boost::shared_ptr<spotify::PlayListElement> playlist = plContainer->GetChild(i);
		switch( playlist->GetType() ){
		case spotify::PlayListElement::PlayListType::PLAYLIST:
			{
				boost::shared_ptr<spotify::PlayList> pl = boost::dynamic_pointer_cast<spotify::PlayList>(playlist);
				while(plContainer->IsLoading(true)) //scan all playlists again to check most current playlist copy
					boost::this_thread::sleep(boost::posix_time::millisec(50));
				scanner_events_duplicates_scanPlaylist(pl, i);
				break;
			}
		case spotify::PlayListElement::PlayListType::PLAYLIST_CONTAINER:
			//scanner_events_duplicates_scanPlaylistContainer(playlist);
			break;
		case spotify::PlayListElement::PlayListType::PLAYLIST_FOLDER:
			//scanner_events_duplicates_scanPlaylistFolder(playlist);
			break;
		}
	}

	std::list<TrackEntry>::iterator it;
	std::string dupMsg;
	dupMsg = boost::str((boost::format("Duplicate List: \n")));

	for(it=duplicateList.begin(); it!=duplicateList.end(); ++it)
	{
		std::string fmtStr = it->GetFormattedInfo();
		dupMsg = boost::str((boost::format("%s\n%s") % dupMsg.c_str() % fmtStr.c_str()));
	}
	message_dialog(dupMsg.c_str());
	

	//Remove duplicates only if setting is checked
	if(g_bRemoveDuplicates)
		scanner_events_duplicates_removeDuplicates();
	else{
		SendMessage(m_hWnd, UWM_SCANNER_SCAN_FINISHED, 0, 0); //Send message indicating finished.
		mtx_scanning.unlock(); //allow other scans
	}
}

//
// Function which will scan a playlist for duplicates!
// Scans an individual playlist and adds duplicates to the global duplicate list. 
void scanner_events_duplicates_scanPlaylist(
	boost::shared_ptr<spotify::PlayList> playlist,
	int plIndex){
	typedef std::map<std::string, TrackEntry> PlMapType;
	//Create a map of this list
	PlMapType plMap;
	//Get pl name
	std::string plName = playlist->GetName();
	
	//Loop through every track in current playlist
	for(int i = 0; i < playlist->GetNumTracks(); i++){
		//Gather and store information about this track in a TrackEntry class.
		boost::shared_ptr<spotify::Track> track = playlist->GetTrack(i);
		std::string trackName = track->GetName();
		std::string trackArtist = track->GetArtist(0)->GetName();
		std::string trackURL = track->GetURL();
		TrackEntry curEntry = TrackEntry(i, trackURL, plName, plIndex, trackArtist, trackName);

		//Setup a return value to show whether element was inserted
		std::pair<PlMapType::iterator,bool> ret;
		ret = plMap.insert(std::make_pair(trackURL, curEntry));
		//If we can't insert into the map, the track is a duplicate.
		if(ret.second == false)
			duplicateList.push_back(curEntry);
	}
	plMap.clear();
	
}

// Function is to be run after a scan and will remove
// Duplicates from the library by looking through 
// the duplicate list
void scanner_events_duplicates_removeDuplicates(){
	std::list<TrackEntry>::iterator it;

	//Loop through all duplicates
	it= duplicateList.begin();
	while( it!=duplicateList.end() )
	{
		sp_error error = SP_ERROR_OTHER_PERMANENT;
		error = it->RemoveTrack();
		
		if(error == SP_ERROR_OK)
		{
			message("Removed track: %s", it->title.c_str());
			it++;
		}
		else{
			message("Permission Denied removing track: %s.", it->title.c_str());
			it++;
		}
	}
	SendMessage(m_hWnd, UWM_SCANNER_SCAN_FINISHED, 0, 0); //Send message indicating finished.
	mtx_scanning.unlock(); //allow other scans
}

//////////////////////////////////
///Playlist Container Callbacks///
//////////////////////////////////
/*
* We use this callback because it allows us to hook the playlist_added call
*/
void SP_CALLCONV scanner_callback_container_loaded(sp_playlistcontainer *pc, void *userdata){
	message("container loaded. scanner_callback_container_loaded");

	//Do not put in thread_pool. This is executed from the threadpool thread already.
	
	sp_playlist_callbacks plCallbacks = getPlaylistCallbacks();
	plContainer->OnContainerLoaded(&plCallbacks);

	//Loading is set to false in OnContainerLoaded(plCallbacks)
	
}

///////////////////////////
//Playlist Callbacks//////
//////////////////////////
void SP_CALLCONV scanner_callback_playlist_tracks_added
	(sp_playlist *pl, sp_track *const *tracks, 
	int num_tracks, int position, void *userdata){
		//Reload the playlist when tracks are added.
		spotify::PlayList * play_list = spotify::PlayList::GetPlayListFromUserData(pl, userdata);
		play_list->LoadTracks();
		//Spotify will call this method for every track as its initially loaded.
		//Therefore we have a firstRunComplete boolean
		if(g_bFirstRunComplete){
			SendMessage(m_hWnd, UWM_SCANNER_SCAN_REQUESTED, 0, 0);
		}
		//else do not send this message
}

void SP_CALLCONV scanner_callbacks_playlist_tracks_removed(sp_playlist *pl, 
																  const int *tracks, int num_tracks,
																  void *userdata)
{
	//We want to force re-downloading of the playlist when tracks are removed so
	//duplicate checking function realizes not try to delete already deleted tracks.
	spotify::PlayList * play_list = 
		spotify::PlayList::GetPlayListFromUserData(pl, userdata);
	play_list->LoadTracks();

}

void SP_CALLCONV scanner_callbacks_playlist_tracks_moved
	(sp_playlist *pl, const int *tracks, int num_tracks,
                                                  int new_position, void *userdata)
{
	//We want to force re-downloading of the playlist when tracks are moved so
	// duplicate checking function realizes that tracks are no longer where
	// they were initially
	spotify::PlayList * play_list = 
		spotify::PlayList::GetPlayListFromUserData(pl, userdata);
	play_list->LoadTracks();

}
