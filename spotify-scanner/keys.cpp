#include "keys.h"

#include <cstdint>
#include <cstddef>

#include <boost/thread/once.hpp>
#include <boost/filesystem.hpp>

const std::uint8_t SpotifyBasicFixture::appkey[] = {
        
};

namespace {
boost::once_flag once = BOOST_ONCE_INIT;
}
SpotifyBasicFixture::SpotifyBasicFixture() : key_size(sizeof(appkey)), username(""), password(""), configuration() {

    configuration.app_key = appkey;
    configuration.app_key_size = key_size;
    configuration.cache_location = "tmp";
    configuration.settings_ocation = "tmp";
    configuration.user_agent = "libspotifypp";


}

SpotifyBasicFixture::~SpotifyBasicFixture() {
}