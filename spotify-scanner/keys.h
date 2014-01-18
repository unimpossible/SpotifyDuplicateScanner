#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>
#include <stdlib.h>
#include "spotify-scanner.h"

class SpotifyBasicFixture{
	static const std::uint8_t SpotifyBasicFixture::appkey[];

	const std::size_t key_size;

public:
	std::string username;
	std::string password;
	spotify::Config configuration;

	SpotifyBasicFixture();
	~SpotifyBasicFixture();
};


extern std::string username;
extern std::string password;

#endif