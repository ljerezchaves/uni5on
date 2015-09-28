/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2013 Federal University of Uberlandia
 *               2015 University of Campinas (Unicamp)
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Saulo da Mata <damata.saulo@gmail.com>
 *         Luciano Chaves <luciano@lrc.ic.unicamp.br>
 */

#ifndef HTTP_HEADER_H_
#define HTTP_HEADER_H_

#include "ns3/header.h"
#include <map>

namespace ns3 {

/**
 * \class HttpHeader.
 * \brief Packet header for HTTP.
 */
class HttpHeader : public Header
{
public:
  /**
   * \brief Construct a null HTTP header.
   */
  HttpHeader ();

  /**
   * \brief Set the message as request.
   */
  void SetRequest (void);

  /**
   * \brief Set the message as response.
   */
  void SetResponse (void);

  /**
   * \brief Query the message for request type.
   * \return True for request messages, false otherwise.
   */
  bool IsRequest (void) const;

  /**
   * \brief Query the message for response type.
   * \return True for response messages, false otherwise.
   */
  bool IsResponse (void) const;

  /**
   * \brief Set the method field of the request message.
   * \param method The string with the request method (GET, HEAD, POST, etc.).
   */
  void SetRequestMethod (std::string method);

  /**
   * \brief Get the method field of the request message.
   * \return The method field.
   */
  std::string GetRequestMethod (void) const;

  /**
   * \brief Set the URL field of the request message.
   * \param url The string with the object URL.
   */
  void SetRequestUrl (std::string url);

  /**
   * \brief Get the URL field of the request message.
   * \return The URL field.
   */
  std::string GetRequestUrl (void) const;

  /**
   * \brief Set the HTTP version field.
   * \param version The string with the version of the HTTP protocol (HTTP/1.0
   * or HTTP/1.1).
   */
  void SetVersion (std::string version);

  /**
   * \brief Get the HTTP version field.
   * \return The version field.
   */
  std::string GetVersion (void) const;

  /**
   * \brief Set the Status Code field of the response message.
   * \param statusCode The string with the status code of the response message
   * (200, 301, 404, etc.).
   */
  void SetResponseStatusCode (std::string statusCode);

  /**
   * \brief Get the status code field of the response message.
   * \return The status code field.
   */
  std::string GetResponseStatusCode (void) const;

  /**
   * \brief Set the phrase field of the response message.
   * \param phrase The string with the phrase of the response message (OK, NOT
   * FOUND, etc.).
   */
  void SetResponsePhrase (std::string phrase);

  /**
   * \brief Get the phrase field of the response message.
   * \return The phrase field.
   */
  std::string GetResponsePhrase (void) const;

  /**
   * \brief Set the Header Field of the HTTP Message
   * \param fieldName The name of the header field.
   * \param fieldValue The value of the header field.
   */
  void SetHeaderField (std::string fieldName, std::string fieldValue);

  /**
   * \brief Set the Header Field of the HTTP Message
   * \param fieldName The name of the header field.
   * \param fieldValue The value of the header field.
   */
  void SetHeaderField (std::string fieldName, uint32_t fieldValue);

  /**
   * \brief Set the Header Field of the HTTP Message
   * \param fieldNameAndValue The name and value of the header field.
   */
  void SetHeaderField (std::string fieldNameAndValue);

  /**
   * \brief Get the Header Field of the HTTP Message
   * \param fieldName The name of the header field.
   * \return the header field
   */
  std::string GetHeaderField (std::string fieldName);

  /**
   * \brief Get the type ID.
   * \return type ID
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  // Inherited from Header
  virtual void Print (std::ostream &os) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);
  virtual uint32_t GetSerializedSize (void) const;

private:
  bool        m_request;    //!< True for request messages, false for response
  std::string m_method;     //!< Request method field
  std::string m_url;        //!< Request URL field
  std::string m_version;    //!< HTTP version field
  std::string m_statusCode; //!< Responde status code
  std::string m_phrase;     //!< Responde phrase field

  /** Map saving header fields and values */
  typedef std::map<std::string, std::string> HeaderFieldMap_t;
  HeaderFieldMap_t m_headerFieldMap;  //!< Map of header fields
};

}

#endif /* HTTP_HEADER_H_ */
