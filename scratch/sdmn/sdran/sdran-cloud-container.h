/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2017 University of Campinas (Unicamp)
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

#ifndef SDRAN_CLOUD_CONTAINER_H
#define SDRAN_CLOUD_CONTAINER_H

#include <vector>
#include "sdran-cloud.h"

namespace ns3 {

/**
 * \ingroup sdmnSdran
 * Keep track of a set of SDRAN cloud pointers.
 */
class SdranCloudContainer
{
public:
  /** Container iterator. */
  typedef std::vector<Ptr<SdranCloud> >::const_iterator Iterator;

  /** Create an empty container. */
  SdranCloudContainer ();

  /**
   * Create a container with exactly one SDRAN cloud object which has been
   * previously instantiated. The single SDRAN cloud object is specified by a
   * smart pointer.
   * \param sdranCloud The Ptr<SdranCloud> to add to the container.
   */
  SdranCloudContainer (Ptr<SdranCloud> sdranCloud);

  /**
   * Create a container with exactly one SDRAN cloud object which has been
   * previously instantiated and assigned a name using the Object Name Service.
   * This SDRAN cloud object is then specified by its assigned name.
   * \param sdranCloudName The name of the SDRAN cloud object to add to the
   *                       container.
   */
  SdranCloudContainer (std::string sdranCloudName);

  /**
   * Create a container which is a concatenation of two input containers.
   * \param a The first container
   * \param b The second container
   *
   * \note A frequently seen idiom that uses these constructors involves the
   * implicit conversion by constructor of Ptr<SdranCloud>. When used, two
   * Ptr<SdranCloud> will be passed to this constructor instead of
   * SdranCloudContainer&. C++ will notice the implicit conversion path that
   * goes through the SdranCloudContainer (Ptr<SdranCloud> node) constructor
   * above. Using this conversion one may optionally provide arguments of
   * Ptr<SdranCloud> to these constructors.
   */
  SdranCloudContainer (const SdranCloudContainer &a,
                       const SdranCloudContainer &b);

  /**
   * Get an iterator which refers to the first SDRAN cloud object in the
   * container. SDRAN cloud object can be retrieved from the container in two
   * ways. First, directly by an index into the container, and second, using an
   * iterator. This method is used in the iterator method and is typically used
   * in a for-loop to run through the SDRAN cloud object.
   *
   * \code
   *   SdranCloudContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some SdranCloud method
   *     }
   * \endcode
   *
   * \returns An iterator which refers to the first SDRAN cloud object in the
   *          container.
   */
  Iterator Begin (void) const;

  /**
   * Get an iterator which indicates past-the-last SDRAN cloud object in the
   * container. SDRAN cloud object can be retrieved from the container in two
   * ways. First, directly by an index into the container, and second, using an
   * iterator. This method is used in the iterator method and is typically used
   * in a for-loop to run through the SDRAN cloud object.
   *
   * \code
   *   SdranCloudContainer::Iterator i;
   *   for (i = container.Begin (); i != container.End (); ++i)
   *     {
   *       (*i)->method ();  // some SdranCloud method
   *     }
   * \endcode
   *
   * \returns An iterator which indicates an ending condition for a loop.
   */
  Iterator End (void) const;

  /**
   * Get the number of Ptr<SdranCloud> stored in this container. SDRAN cloud
   * object can be retrieved from the container in two ways. First, directly by
   * an index into the container, and second, using an iterator. This method is
   * used in the direct method and is typically used to define an ending
   * condition in a for-loop that runs through the stored SDRAN cloud object.
   *
   * \code
   *   uint32_t nObjects = container.GetN ();
   *   for (uint32_t i = 0 i < nObjects; ++i)
   *     {
   *       Ptr<SdranCloud> p = container.Get (i)
   *       i->method ();  // some SdranCloud method
   *     }
   * \endcode
   *
   * \returns The number of Ptr<SdranCloud> stored in this container.
   */
  uint32_t GetN (void) const;

  /**
   * Get the Ptr<SdranCloud> stored in this container at a given index. Get the
   * number of Ptr<SdranCloud> stored in this container. SDRAN cloud object can
   * be retrieved from the container in two ways. First, directly by an index
   * into the container, and second, using an iterator. This method is used in
   * the direct method and is used to retrieve the indexed Ptr<SdranCloud>.
   *
   * \code
   *   uint32_t nObjects = container.GetN ();
   *   for (uint32_t i = 0 i < nObjects; ++i)
   *     {
   *       Ptr<SdranCloud> p = container.Get (i)
   *       i->method ();  // some SdranCloud method
   *     }
   * \endcode
   *
   * \param i The index of the requested object pointer.
   * \returns The requested object pointer.
   */
  Ptr<SdranCloud> Get (uint32_t i) const;

  /**
   * Create n SDRAN cloud object and append pointers to them to the end of this
   * SdranCloudContainer.
   * \param n The number of SDRAN cloud object to create
   */
  void Create (uint32_t n);

  /**
   * Append the contents of another SdranCloudContainer to the end of this
   * container.
   * \param other The SdranCloudContainer to append.
   */
  void Add (SdranCloudContainer other);

  /**
   * Append a single Ptr<SdranCloud> to this container.
   * \param sdranCloud The Ptr<SdranCloud> to append.
   */
  void Add (Ptr<SdranCloud> sdranCloud);

  /**
   * Append to this container the single Ptr<SdranCloud> referred to via its
   * object name service registered name.
   * \param sdranCloudName The name of the SDRAN cloud object to add to the
   *                       container.
   */
  void Add (std::string sdranCloudName);

private:
  std::vector<Ptr<SdranCloud> > m_objects;  //!< SDRAN cloud smart pointers
};

} // namespace ns3

#endif /* SDRAN_CLOUD_CONTAINER_H */
