/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License
 * as published by the Free Software Foundation, either version 3 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#include "DecodedImageAdapter.h"

#include "ViewerToolbox.h"
#include "ParsedDicomImage.h"

#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <json/writer.h>

namespace OrthancPlugins
{
  bool DecodedImageAdapter::ParseUri(CompressionType& type,
                                     uint8_t& compressionLevel,
                                     std::string& instanceId,
                                     const std::string& uri)
  {
    size_t separator = uri.find('-');
    if (separator == std::string::npos &&
        separator >= 1)
    {
      return false;
    }
  
    std::string compression = uri.substr(0, separator);
    instanceId = uri.substr(separator + 1);

    if (compression == "deflate")
    {
      type = CompressionType_Deflate;
    }
    else if (boost::starts_with(compression, "jpeg"))
    {
      type = CompressionType_Jpeg;
      int level = boost::lexical_cast<int>(compression.substr(4));
      if (level <= 0 || level > 100)
      {
        return false;
      }

      compressionLevel = static_cast<uint8_t>(level);
    }
    else
    {
      return false;
    }

    return true;
  }



  bool DecodedImageAdapter::Create(std::string& content,
                                   const std::string& uri)
  {
    std::string message = "Decoding DICOM instance: " + uri;
    OrthancPluginLogInfo(context_, message.c_str());

    CompressionType type;
    uint8_t level;
    std::string instanceId;
    
    if (!ParseUri(type, level, instanceId, uri))
    {
      return false;
    }

    ParsedDicomImage image(context_, instanceId);

    Json::Value json;
    bool ok = false;

    if (type == CompressionType_Deflate)
    {
      ok = image.EncodeUsingDeflate(json, 9);
    }
    else if (type == CompressionType_Jpeg)
    {
      ok = image.EncodeUsingJpeg(json, level);
    }

    if (ok)
    {
      Json::FastWriter writer;
      content = writer.write(json);
      return true;
    }
    else
    {
      char msg[1024];
      sprintf(msg, "Unable to decode the following instance: %s", uri.c_str());
      OrthancPluginLogWarning(context_, msg);
      return false;
    }
  }
}
