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

#include "ns3/log.h"
#include "http-header.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("HttpHeader");
NS_OBJECT_ENSURE_REGISTERED (HttpHeader);

HttpHeader::HttpHeader ()
  : m_request (true),
    m_method (""),
    m_url (""),
    m_version (""),
    m_statusCode (""),
    m_phrase ("")
{
  NS_LOG_FUNCTION_NOARGS ();
}

void
HttpHeader::SetRequest (void)
{
  NS_LOG_FUNCTION (this);
  m_request = true;
}

void
HttpHeader::SetResponse (void)
{
  NS_LOG_FUNCTION (this);
  m_request = false;
}

bool
HttpHeader::IsRequest (void) const
{
  NS_LOG_FUNCTION (this);
  return m_request;
}

bool
HttpHeader::IsResponse (void) const
{
  NS_LOG_FUNCTION (this);
  return !m_request;
}

void
HttpHeader::SetRequestMethod (std::string method)
{
  NS_LOG_FUNCTION (this << method);
  m_method = method;
}

std::string
HttpHeader::GetRequestMethod (void) const
{
  NS_LOG_FUNCTION (this);
  return m_method;
}

void
HttpHeader::SetRequestUrl (std::string url)
{
  NS_LOG_FUNCTION (this << url);
  m_url = url;
}

std::string
HttpHeader::GetRequestUrl (void) const
{
  NS_LOG_FUNCTION (this);
  return m_url;
}

void
HttpHeader::SetVersion (std::string version)
{
  NS_LOG_FUNCTION (this << version);
  m_version = version;
}

std::string
HttpHeader::GetVersion (void) const
{
  NS_LOG_FUNCTION (this);
  return m_version;
}

void
HttpHeader::SetResponseStatusCode (std::string statusCode)
{
  NS_LOG_FUNCTION (this << statusCode);
  m_statusCode = statusCode;
}

std::string
HttpHeader::GetResponseStatusCode (void) const
{
  NS_LOG_FUNCTION (this);
  return m_statusCode;
}

void
HttpHeader::SetResponsePhrase (std::string phrase)
{
  NS_LOG_FUNCTION (this << phrase);
  m_phrase = phrase;
}

std::string
HttpHeader::GetResponsePhrase (void) const
{
  NS_LOG_FUNCTION (this);
  return m_phrase;
}

void
HttpHeader::SetHeaderField (std::string fieldName, std::string fieldValue)
{
  NS_LOG_FUNCTION (this << fieldName << ": " << fieldValue);

  HeaderFieldMap_t::iterator it = m_headerFieldMap.find (fieldName);
  if (it == m_headerFieldMap.end ())
    {
      std::pair <std::string, std::string> entry (fieldName, fieldValue);
      m_headerFieldMap.insert (entry);
    }
  else
    {
      it->second = fieldValue;
    }
}

void
HttpHeader::SetHeaderField (std::string fieldName, uint32_t fieldValue)
{
  std::stringstream fieldValueSs;
  fieldValueSs << fieldValue;
  SetHeaderField (fieldName, fieldValueSs.str ());
}

void
HttpHeader::SetHeaderField (std::string fieldNameAndValue)
{
  NS_LOG_FUNCTION (this << fieldNameAndValue);

  uint16_t middle = fieldNameAndValue.find_first_of (":");
  uint16_t end = fieldNameAndValue.length () - 1;
  std::string fieldName = fieldNameAndValue.substr (0, middle);
  std::string fieldValue = fieldNameAndValue.substr (middle + 2, end);

  SetHeaderField (fieldName, fieldValue);
}

std::string
HttpHeader::GetHeaderField (std::string fieldName)
{
  NS_LOG_FUNCTION (this << fieldName);

  HeaderFieldMap_t::iterator it = m_headerFieldMap.find (fieldName);
  if (it != m_headerFieldMap.end ())
    {
      return it->second;
    }
  else
    {
      NS_LOG_ERROR ("Header Field: " << fieldName << " does not exist. "
                    "It has not been set by the remote side.");
      return "";
    }
}

TypeId
HttpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::HttpHeader")
    .SetParent<Header> ()
    .AddConstructor<HttpHeader> ()
  ;
  return tid;
}

TypeId
HttpHeader::GetInstanceTypeId (void) const
{
  NS_LOG_FUNCTION (this);
  return GetTypeId ();
}

void
HttpHeader::Print (std::ostream &os) const
{
  NS_LOG_FUNCTION (this);

  if (IsRequest ())
    {
      os << m_method << " " << m_url << " " << m_version << "\n";
    }
  else
    {
      os << m_version << " " << m_statusCode << " " << m_phrase << "\n";
    }

  HeaderFieldMap_t::const_iterator it;
  for (it = m_headerFieldMap.begin (); it != m_headerFieldMap.end (); it++)
    {
      os << it->first << ": " << it->second << "\n";
    }

  os << "\n";
}

uint32_t
HttpHeader::GetSerializedSize (void) const
{
  NS_LOG_FUNCTION (this);

  uint32_t size = 0;
  if (IsRequest ())
    {
      // Request line
      size += m_method.length ();
      size += m_url.length ();
      size += m_version.length ();
      size += 4; // spaces and CR LF
    }
  else
    {
      // Status line
      size += m_version.length ();
      size += m_statusCode.length ();
      size += m_phrase.length ();
      size += 4; // spaces and CR LF
    }

  // Header lines
  HeaderFieldMap_t::const_iterator it;
  for (it = m_headerFieldMap.begin (); it != m_headerFieldMap.end (); it++)
    {
      size += it->first.length () + it->second.length ();
      size += 4; // spaces and CR LF
    }

  // Blank line
  size += 2;

  return size;
}

void
HttpHeader::Serialize (Buffer::Iterator start) const
{
  NS_LOG_FUNCTION (this);

  std::string lines;
  if (IsRequest ())
    {
      // Request line
      lines = m_method + " " + m_url + " " + m_version + "\r\n";
    }
  else
    {
      // Status line
      lines = m_version + " " + m_statusCode + " " + m_phrase + "\r\n";
    }

  // Header lines
  HeaderFieldMap_t::const_iterator it;
  for (it = m_headerFieldMap.begin (); it != m_headerFieldMap.end (); it++)
    {
      lines += it->first + ": " + it->second + "\r\n";
    }

  // Blank line
  lines += "\r\n";

  char tmpBuffer [lines.length () + 1];
  strcpy (tmpBuffer, lines.c_str ());
  start.Write ((uint8_t*)tmpBuffer, strlen (tmpBuffer));
}

uint32_t
HttpHeader::Deserialize (Buffer::Iterator start)
{
  NS_LOG_FUNCTION_NOARGS ();

  // Checking the length of HTTP header (finishes with \r\n\r\n string).
  uint8_t c;
  uint32_t length = 4;
  uint32_t checkEnd;
  Buffer::Iterator startCopy = start;
  checkEnd = startCopy.ReadU32 ();
  while (checkEnd != 0x0d0a0d0a)  // equivalent to \r\n\r\n
    {
      c = startCopy.ReadU8 ();
      checkEnd <<= 8;
      checkEnd |= static_cast<uint32_t> (c);
      length++;
    }
  char tmpBuffer [length + 1];
  start.Read ((uint8_t*)tmpBuffer, length);
  tmpBuffer [length] = '\0';
  std::string httpString (tmpBuffer);

  // Tokenizer...
  uint16_t begin = 0, end = 0;
  end = httpString.find_first_of (" ");
  std::string firstField = httpString.substr (begin, end - begin);

  if (firstField == "GET")
    {
      SetRequest ();
    }
  else
    {
      SetResponse ();
    }

  if (IsRequest ())
    {
      m_method = firstField;

      begin = end + 1;
      end = httpString.find_first_of (" ", begin);
      m_url = httpString.substr (begin, end - begin);

      begin = end + 1;
      end = httpString.find_first_of ("\r\n", begin);
      m_version = httpString.substr (begin, end - begin);
    }
  else
    {
      m_version = firstField;

      begin = end + 1;
      end = httpString.find_first_of (" ", begin);
      m_statusCode = httpString.substr (begin, end - begin);

      begin = end + 1;
      end = httpString.find_first_of ("\r\n", begin);
      m_phrase = httpString.substr (begin, end - begin);
    }

  begin = end + 2;
  end = httpString.find_first_of ("\r\n", begin);
  while (begin != end && end != std::string::npos)
    {
      std::string headerField = httpString.substr (begin, end - begin);
      SetHeaderField (headerField);

      begin = end + 2;
      end = httpString.find_first_of ("\r\n", begin);
    }

  uint32_t thisSize = GetSerializedSize ();
  NS_ASSERT_MSG (length == thisSize, "Error deserializing HTTP header.");

  return thisSize;
}

} // namespace ns3
