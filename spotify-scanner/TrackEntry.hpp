#ifndef SPOTIFY_SCANNER_TRACKENTRY_H
#define SPOTIFY_SCANNER_TRACKENTRY_H

#include "spotify-scanner.h"
/**
 * A class representing each entry in a playlist.
 * It is used to represent the metadata in each 
 * track as it is retrieved from the spotify library
 */
class TrackEntry
{
public:
	int trackID;
	std::string artist;
	std::string title;
	std::string url;
	std::string playlist;
	int plIndex;

	//Set to true if we tried to remove already and the permission was denied (aka pl not owned by user)
	bool permissionDenied; 

	//If deleted
	bool deleted;

	/** 
	 * The only constructor for this object.
	 * @param id Track ID inside the playlist (order of the track)
	 * @param url URL of the track
	 * @param plName Name of the playlist containing the track
	 * @param plIndex the index of the playlist inside the library
	 * @param artist String of the artist name
	 * @param title
	 */
	TrackEntry(int id, std::string url, std::string plName, int plIndex, std::string artist, std::string title);
	virtual ~TrackEntry(){};

	sp_error RemoveTrack();

	std::string GetFormattedInfo(){
		return boost::str(boost::format("Track Name: %s, Artist: %s, Found in Playlist: %s at %d") % title % artist % playlist % trackID);
	}

	bool operator== (const TrackEntry& b);

};

#endif