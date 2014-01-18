/*
 * Copyright 2011 Jim Knowler
 *           2012 Alexander Rojas
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#pragma once

// libspotify includes
#include <libspotify/api.h>

// std include
#include <string>

// boost includes
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>

// Local includes
#include "spotify/LibConfig.hpp"
#include "spotify/ArtistBrowse.hpp"

namespace spotify {
// forward declaration
class Session;
class ArtistBrowse;

class LIBSPOTIFYPP_API Artist : public boost::enable_shared_from_this<Artist> {
  public:
    explicit Artist(boost::shared_ptr<Session> session);
    virtual ~Artist();

    void Load(sp_artist *pArtist);
    bool IsLoading();

    std::string GetName();

    boost::shared_ptr<ArtistBrowse> Browse();

  protected:
    friend class ArtistBrowse;

    sp_artist *artist_;
    boost::shared_ptr<Session> session_;
};
}
