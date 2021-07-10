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

#include "movie-helper.h"

namespace ns3 {

const std::vector<MovieHelper::VideoTrace> MovieHelper::m_videos = {
  {.name = "./movies/office-cam-low.txt",       .gbr = DataRate ( 120000), .mbr = DataRate ( 128000)},
  {.name = "./movies/office-cam-medium.txt",    .gbr = DataRate ( 128000), .mbr = DataRate ( 600000)},
  {.name = "./movies/office-cam-high.txt",      .gbr = DataRate ( 450000), .mbr = DataRate ( 500000)},
  {.name = "./movies/first-contact.txt",        .gbr = DataRate ( 400000), .mbr = DataRate ( 650000)},
  {.name = "./movies/star-wars-iv.txt",         .gbr = DataRate ( 500000), .mbr = DataRate ( 600000)},
  {.name = "./movies/ard-talk.txt",             .gbr = DataRate ( 500000), .mbr = DataRate ( 700000)},
  {.name = "./movies/mr-bean.txt",              .gbr = DataRate ( 600000), .mbr = DataRate ( 800000)},
  {.name = "./movies/n3-talk.txt",              .gbr = DataRate ( 650000), .mbr = DataRate ( 750000)},
  {.name = "./movies/the-firm.txt",             .gbr = DataRate ( 700000), .mbr = DataRate ( 800000)},
  {.name = "./movies/ard-news.txt",             .gbr = DataRate ( 750000), .mbr = DataRate (1250000)},
  {.name = "./movies/jurassic-park.txt",        .gbr = DataRate ( 770000), .mbr = DataRate (1000000)},
  {.name = "./movies/from-dusk-till-dawn.txt",  .gbr = DataRate ( 800000), .mbr = DataRate (1000000)},
  {.name = "./movies/formula1.txt",             .gbr = DataRate (1100000), .mbr = DataRate (1200000)},
  {.name = "./movies/soccer.txt",               .gbr = DataRate (1300000), .mbr = DataRate (1500000)},
  {.name = "./movies/silence-of-the-lambs.txt", .gbr = DataRate (1500000), .mbr = DataRate (2000000)}
};

Ptr<UniformRandomVariable> MovieHelper::m_gbrVidRng =
  CreateObjectWithAttributes<UniformRandomVariable> (
    "Min", DoubleValue (0), "Max", DoubleValue (2));

Ptr<UniformRandomVariable> MovieHelper::m_nonVidRng =
  CreateObjectWithAttributes<UniformRandomVariable> (
    "Min", DoubleValue (3), "Max", DoubleValue (14));

MovieHelper::MovieHelper ()
{
}

const MovieHelper::VideoTrace
MovieHelper::GetRandomVideo (QosType type)
{
  switch (type)
    {
    case QosType::NON:
      return m_videos.at (m_nonVidRng->GetInteger ());
    case QosType::GBR:
      return m_videos.at (m_gbrVidRng->GetInteger ());
    default:
      NS_ABORT_MSG ("Invalid QoS traffic type.");
      return VideoTrace {};
    }
}

} // namespace ns3
