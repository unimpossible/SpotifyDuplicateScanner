#include "TrackEntry.hpp"

//All playlists have been scanned. Report outcome.
	
//Constructor
//Sets all parameters necessary.
TrackEntry::TrackEntry(
	int id,		
	std::string url_, 
	std::string plName, 
	int plIndex_,
	std::string artist_, 
	std::string title_)
{
	trackID = id;
	playlist = plName;
	plIndex = plIndex_;
	artist = artist_;
	url = url_;
	title = title_;

	permissionDenied = false; //Initially we haven't tried yet.
	deleted = false;
}

//
// Function will remove a track from the playlist.
// Uses data stored in the object to lookup the playlist
// and remove the track.
sp_error TrackEntry::RemoveTrack(){
	//Check and see if we already tried to remove this track.
	if(permissionDenied)
		return SP_ERROR_PERMISSION_DENIED;
	//Allows list to continue to expand while retaining history.
	if(deleted)
		return SP_ERROR_OK;

	sp_error error;
	//Check to make sure we have the plContainer initialized
	if(!plContainer)
		return SP_ERROR_INVALID_INDATA;

	boost::shared_ptr<spotify::PlayListElement> playlist = plContainer->GetChild(plIndex);
	if(!playlist)
		return SP_ERROR_INVALID_INDATA;

	//Cast the playlistElement to a playlist type.
	boost::shared_ptr<spotify::PlayList> pl = boost::dynamic_pointer_cast<spotify::PlayList>(playlist);

	if(!pl)
		return SP_ERROR_INVALID_INDATA;

	//Call the new method to remove this track using the trackID
	error = pl->RemoveTrack(trackID);

	//Set flag
	if(error == SP_ERROR_PERMISSION_DENIED)
		permissionDenied = true;
	else if(error == SP_ERROR_OK)
		deleted = true;

	return error;
}

bool TrackEntry::operator== (const TrackEntry& b)
{
	if(this->url == b.url)
		return true;
	else
		return false;
}