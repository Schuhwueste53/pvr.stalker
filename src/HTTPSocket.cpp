/*
 *      Copyright (C) 2015  Jamal Edey
 *      http://www.kenshisoft.com/
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 *
 */

#include "HTTPSocket.h"

#include "kodi/util/StringUtils.h"

#include "client.h"

#define TEMP_BUFFER_SIZE 1024

using namespace ADDON;
using namespace PLATFORM;

HTTPSocket::HTTPSocket()
{
  UrlOption option;

  option = { "User-Agent", "Mozilla/5.0 (QtEmbedded; U; Linux; C) AppleWebKit/533.3 (KHTML, like Gecko) MAG200 stbapp ver: 2 rev: 250 Safari/533.3" };
  m_defaultOptions.push_back(option);

  option = { "Connection-Timeout", "5" };
  m_defaultOptions.push_back(option);
}

HTTPSocket::~HTTPSocket()
{
  m_defaultOptions.clear();
}

void HTTPSocket::SetDefaults(Request *request)
{
  bool found;
  
  for (std::vector<UrlOption>::iterator option = m_defaultOptions.begin(); option != m_defaultOptions.end(); ++option) {
    found = false;

    for (std::vector<UrlOption>::iterator it = request->options.begin(); it != request->options.end(); ++it) {
      std::string oname = option->name; StringUtils::ToLower(oname);
      std::string iname = it->name; StringUtils::ToLower(iname);
      
      if (found = (iname.compare(oname) == 0))
        break;
    }

    if (!found)
      request->AddHeader(option->name, option->value);
  }
}

void HTTPSocket::BuildRequestUrl(Request *request, std::string *strRequestUrl)
{
  char buffer[TEMP_BUFFER_SIZE];
  
  SetDefaults(request);
  
  strRequestUrl->append(request->url + "|");
  
  for (std::vector<UrlOption>::iterator it = request->options.begin(); it != request->options.end(); ++it) {
    sprintf(buffer, "%s=%s", it->name.c_str(), it->value.c_str());
    strRequestUrl->append(buffer);
    
    if (it + 1 != request->options.end())
      strRequestUrl->append("&");
  }
}

bool HTTPSocket::Get(std::string *strRequestUrl, std::string *strResponse)
{
  void *hdl;
  char buffer[1024];
  
  hdl = XBMC->OpenFile(strRequestUrl->c_str(), 0);
  if (hdl) {
    memset(buffer, 0, sizeof(buffer));
    while (XBMC->ReadFileString(hdl, buffer, sizeof(buffer) - 1)) {
      strResponse->append(buffer);
      memset(buffer, 0, sizeof(buffer));
    }
    
    XBMC->CloseFile(hdl);
  }
  
  return true;
}

bool HTTPSocket::Execute(Request *request, Response *response)
{
  std::string strRequestUrl;
  bool result;

  BuildRequestUrl(request, &strRequestUrl);
  
  switch (request->method) {
    case GET:
      result = Get(&strRequestUrl, &response->body);
      break;
    //case POST: //TODO
  }
  
  if (!result) {
    XBMC->Log(LOG_ERROR, "%s: request failed", __FUNCTION__);
    return false;
  }
  
  XBMC->Log(LOG_DEBUG, "%s", response->body.substr(0, 512).c_str()); // 512 is max
  
  return true;
}

HTTPSocketRaw::HTTPSocketRaw()
  : HTTPSocket()
{
}

HTTPSocketRaw::~HTTPSocketRaw()
{
}

void HTTPSocketRaw::BuildRequestString(Request *request, std::string *strRequest)
{
  std::string strMethod;
  std::string strUri;
  size_t pos;
  char buffer[TEMP_BUFFER_SIZE];
  
  SetDefaults(request);
  
  // defaults
  strMethod = "GET";
  strUri = "/";
  m_host = request->url;
  m_port = 80;
  
  switch (request->method) {
    case GET:
      strMethod = "GET";
      break;
    //case POST: //TODO
  }
  
  if (m_host.find("http://") == 0) {
    m_host.replace(0, 7, "");
  }
  if ((pos = m_host.find("/")) != std::string::npos) {
    strUri = m_host.substr(pos);
    m_host.replace(pos, std::string::npos, "");
  }
  if ((pos = m_host.find(":")) != std::string::npos) {
    std::string sport = m_host.substr(pos + 1);
    long int lport = strtol(sport.c_str(), NULL, 10);
    m_port = lport != 0L ? (int)lport : m_port;
    m_host.replace(pos, std::string::npos, "");
  }

  sprintf(buffer, "%s %s HTTP/1.0\r\n", strMethod.c_str(), strUri.c_str());
  strRequest->append(buffer);

  sprintf(buffer, "Host: %s:%d\r\n", m_host.c_str(), m_port);
  strRequest->append(buffer);

  sprintf(buffer, "Accept: %s\r\n", "*/*");
  strRequest->append(buffer);

  for (std::vector<UrlOption>::iterator it = request->options.begin(); it != request->options.end(); ++it) {
    sprintf(buffer, "%s: %s\r\n", it->name.c_str(), it->value.c_str());
    strRequest->append(buffer);
  }

  strRequest->append("\r\n\r\n");

  strRequest->append(request->body);
}

bool HTTPSocketRaw::Open()
{
  uint64_t iNow;
  uint64_t iTarget;

  iNow = GetTimeMs();
  iTarget = iNow + 5 * 1000;

  m_socket = new CTcpConnection(m_host.c_str(), m_port);

  while (!m_socket->IsOpen() && iNow < iTarget) {
    if (!m_socket->Open(iTarget - iNow))
      CEvent::Sleep(100);
    iNow = GetTimeMs();
  }

  if (!m_socket->IsOpen())
    return false;

  return true;
}

void HTTPSocketRaw::Close()
{
  if (m_socket->IsOpen())
    m_socket->Close();

  if (m_socket) {
    delete m_socket;
    m_socket = NULL;
  }
}

bool HTTPSocketRaw::Execute(Request *request, Response *response)
{
  std::string strRequest;
  std::string strResponse;
  char buffer[TEMP_BUFFER_SIZE];
  int len;
  size_t pos;
  
  BuildRequestString(request, &strRequest);

  if (!Open()) {
    XBMC->Log(LOG_ERROR, "%s: failed to connect", __FUNCTION__, m_socket->GetError().c_str());
    return false;
  }

  if ((len = m_socket->Write((void*)strRequest.c_str(), strlen(strRequest.c_str()))) < 0) {
    XBMC->Log(LOG_ERROR, "%s: failed to write request", __FUNCTION__);
    return false;
  }

  memset(buffer, 0, TEMP_BUFFER_SIZE);
  while ((len = m_socket->Read(buffer, TEMP_BUFFER_SIZE - 1)) > 0) {
    strResponse.append(buffer, len);
    memset(buffer, 0, TEMP_BUFFER_SIZE);
  }

  Close();

  if ((pos = strResponse.find("\r\n\r\n")) == std::string::npos) {
    XBMC->Log(LOG_ERROR, "%s: failed to parse response", __FUNCTION__);
    return false;
  }

  response->headers = strResponse.substr(0, pos);
  response->body = strResponse.substr(pos + 4);

  XBMC->Log(LOG_DEBUG, "%s", response->headers.c_str());
  XBMC->Log(LOG_DEBUG, "%s", response->body.substr(0, 512).c_str()); // 512 is max

  return true;
}
