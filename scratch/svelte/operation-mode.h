/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2018 University of Campinas (Unicamp)
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
 * Author: Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef OPERATION_MODE_H
#define OPERATION_MODE_H

#include <string>

namespace ns3 {

/**
 * \ingroup svelteInfra
 * Enumeration of available operation modes.
 */
typedef enum
{
  OFF  = 0,   //!< Always off.
  ON   = 1,   //!< Always on.
  AUTO = 2    //!< Automatic.
} OperationMode;

/**
 * \ingroup svelteInfra
 * Get the operation mode name.
 * \param mode The operation mode.
 * \return The string with the operation mode name.
 */
std::string OperationModeStr (OperationMode mode);

} // namespace ns3
#endif // OPERATION_MODE_H