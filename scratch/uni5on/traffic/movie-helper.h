/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2021 University of Campinas (Unicamp)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Luciano Jerez Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef MOVIE_HELPER_H
#define MOVIE_HELPER_H

#include <ns3/core-module.h>
#include "../uni5on-common.h"

namespace ns3 {

/**
 * \ingroup uni5onApps
 * The helper handles MPEG trace file metadata.
 */
class MovieHelper
{
public:
  /** Metadata information for a video trace file. */
  struct VideoTrace
  {
    std::string name;   //!< Filename
    DataRate    gbr;    //!< Guaranteed bit rate
    DataRate    mbr;    //!< Maximum bit rate
  };

  MovieHelper ();       //!< Default constructor.

  /**
   * Get a random video for a given QoS type.
   * \param type The QoS type.
   * \return The video metadata.
   */
  static const VideoTrace GetRandomVideo (QosType type);

private:
  static Ptr<UniformRandomVariable>     m_gbrVidRng;  //!< GBR random.
  static Ptr<UniformRandomVariable>     m_nonVidRng;  //!< Non-GBR random.
  static const std::vector<VideoTrace>  m_videos;     //!< Video information.
};

} // namespace ns3
#endif // MOVIE_HELPER_H
