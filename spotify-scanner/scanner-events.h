#ifndef SPOTIFY_SCANNER_EVENTS_H
#define SPOTIFY_SCANNER_EVENTS_H
#include "spotify-scanner.h"
#include "TrackEntry.hpp"


void scanner_events_initLogging();
sp_error scanner_events_initSession();
void scanner_events_try_remembered_login();
void scanner_events_login();
void scanner_events_fail();
void scanner_events_spawn_UpdateScheduler();
bool scanner_remove_old_data();
sp_error scanner_events_logout();


//Scanner events functions
void SP_CALLCONV scanner_on_connection_error(sp_session *session, sp_error error);
void SP_CALLCONV scanner_on_log_message(sp_session *session, const char *data);
void SP_CALLCONV scanner_callback_logged_in(sp_session *session, sp_error error);



//Duplicate list from all playlists!
extern std::list<TrackEntry> duplicateList;
//Duplicate Scanning Functions
void scanner_events_duplicates_startScan();
void scanner_events_duplicates_runScan();

void scanner_events_duplicates_scanPlaylist(boost::shared_ptr<spotify::PlayList> playlist, int plIndex);
void scanner_events_duplicates_scanPlaylistContainer(boost::shared_ptr<spotify::PlayListElement> playlist);
void scanner_events_duplicates_scanPlaylistFolder(boost::shared_ptr<spotify::PlayListElement> playlist);
void scanner_events_duplicates_removeDuplicates();

/*
	Callback functions
*/

//Callback getters
sp_session_callbacks getSessionCallbacks();
sp_playlist_callbacks getPlaylistCallbacks();

//Playlist Container Callbacks
void SP_CALLCONV scanner_callback_container_loaded(sp_playlistcontainer *pc, void *userdata);

//Playlist Callbacks
void SP_CALLCONV scanner_callback_playlist_state_changed(sp_playlist *pl, void *userdata);

void SP_CALLCONV scanner_callback_playlist_tracks_added(sp_playlist *pl, sp_track *const *tracks, int num_tracks, int position,
                                     void *userdata);

void SP_CALLCONV scanner_callbacks_playlist_tracks_removed(sp_playlist *pl, 
																  const int *tracks, int num_tracks,
																  void *userdata);

void SP_CALLCONV scanner_callbacks_playlist_tracks_moved(sp_playlist *pl, const int *tracks, int num_tracks,
                                                  int new_position, void *userdata);


#endif