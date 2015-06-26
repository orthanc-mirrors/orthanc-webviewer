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


#include "ViewerToolbox.h"

#include "../Orthanc/Core/OrthancException.h"
#include "../Orthanc/Core/Toolbox.h"

#include <json/reader.h>
#include <zlib.h>
#include <stdexcept>
#include <boost/lexical_cast.hpp>
#include <sys/stat.h>

namespace OrthancPlugins
{
  bool GetStringFromOrthanc(std::string& content,
                            OrthancPluginContext* context,
                            const std::string& uri)
  {
    OrthancPluginMemoryBuffer answer;

    if (OrthancPluginRestApiGet(context, &answer, uri.c_str()))
    {
      return false;
    }

    if (answer.size)
    {
      try
      {
        content.assign(reinterpret_cast<const char*>(answer.data), answer.size);
      }
      catch (std::bad_alloc&)
      {
        OrthancPluginFreeMemoryBuffer(context, &answer);
        throw Orthanc::OrthancException("Not enough memory");
      }
    }

    OrthancPluginFreeMemoryBuffer(context, &answer);
    return true;
  }


  bool GetJsonFromOrthanc(Json::Value& json,
                          OrthancPluginContext* context,
                          const std::string& uri)
  {
    OrthancPluginMemoryBuffer answer;

    if (OrthancPluginRestApiGet(context, &answer, uri.c_str()))
    {
      return false;
    }

    if (answer.size)
    {
      try
      {
        const char* data = reinterpret_cast<const char*>(answer.data);
        Json::Reader reader;
        if (!reader.parse(data, data + answer.size, json, 
                          false /* don't collect comments */))
        {
          return false;
        }
      }
      catch (std::runtime_error&)
      {
        OrthancPluginFreeMemoryBuffer(context, &answer);
        return false;
      }
    }

    OrthancPluginFreeMemoryBuffer(context, &answer);
    return true;
  }




  bool TokenizeVector(std::vector<float>& result,
                      const std::string& value,
                      unsigned int expectedSize)
  {
    std::vector<std::string> tokens;
    Orthanc::Toolbox::TokenizeString(tokens, value, '\\');

    if (tokens.size() != expectedSize)
    {
      return false;
    }

    result.resize(tokens.size());

    for (size_t i = 0; i < tokens.size(); i++)
    {
      try
      {
        result[i] = boost::lexical_cast<float>(tokens[i]);
      }
      catch (boost::bad_lexical_cast&)
      {
        return false;
      }
    }

    return true;
  }


  bool CompressUsingDeflate(std::string& compressed,
                            const void* uncompressed,
                            size_t uncompressedSize,
                            uint8_t compressionLevel)
  {
    if (uncompressedSize == 0)
    {
      compressed.clear();
      return true;
    }

    uLongf compressedSize = compressBound(uncompressedSize);
    compressed.resize(compressedSize);

    int error = compress2
      (reinterpret_cast<uint8_t*>(&compressed[0]),
       &compressedSize,
       const_cast<Bytef *>(static_cast<const Bytef *>(uncompressed)), 
       uncompressedSize,
       compressionLevel);

    if (error == Z_OK)
    {
      compressed.resize(compressedSize);
      return true;
    }
    else
    {
      compressed.clear();
      return false;
    }
  }



  const char* GetMimeType(const std::string& path)
  {
    size_t dot = path.find_last_of('.');

    std::string extension = (dot == std::string::npos) ? "" : path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(), tolower);

    if (extension == ".html")
    {
      return "text/html";
    }
    else if (extension == ".css")
    {
      return "text/css";
    }
    else if (extension == ".js")
    {
      return "application/javascript";
    }
    else if (extension == ".gif")
    {
      return "image/gif";
    }
    else if (extension == ".svg")
    {
      return "image/svg+xml";
    }
    else if (extension == ".json")
    {
      return "application/json";
    }
    else if (extension == ".xml")
    {
      return "application/xml";
    }
    else if (extension == ".png")
    {
      return "image/png";
    }
    else if (extension == ".jpg" || extension == ".jpeg")
    {
      return "image/jpeg";
    }
    else
    {
      return "application/octet-stream";
    }
  }


  bool ReadConfiguration(Json::Value& configuration,
                         OrthancPluginContext* context)
  {
    std::string s;

    {
      char* tmp = OrthancPluginGetConfiguration(context);
      if (tmp == NULL)
      {
        OrthancPluginLogError(context, "Error while retrieving the configuration from Orthanc");
        return false;
      }

      s.assign(tmp);
      OrthancPluginFreeString(context, tmp);      
    }

    Json::Reader reader;
    if (reader.parse(s, configuration))
    {
      return true;
    }
    else
    {
      OrthancPluginLogError(context, "Unable to parse the configuration");
      return false;
    }
  }


  std::string GetStringValue(const Json::Value& configuration,
                             const std::string& key,
                             const std::string& defaultValue)
  {
    if (configuration.type() != Json::objectValue ||
        !configuration.isMember(key) ||
        configuration[key].type() != Json::stringValue)
    {
      return defaultValue;
    }
    else
    {
      return configuration[key].asString();
    }
  }  


  int GetIntegerValue(const Json::Value& configuration,
                      const std::string& key,
                      int defaultValue)
  {
    if (configuration.type() != Json::objectValue ||
        !configuration.isMember(key) ||
        configuration[key].type() != Json::intValue)
    {
      return defaultValue;
    }
    else
    {
      return configuration[key].asInt();
    }
  }


  bool ReadFile(std::string& content,
                const std::string& path)
  {
    struct stat s;
    if (stat(path.c_str(), &s) != 0 ||
        !(s.st_mode & S_IFREG))
    {
      // Either the path does not exist, or it is not a regular file
      return false;
    }

    FILE* fp = fopen(path.c_str(), "rb");
    if (fp == NULL)
    {
      return false;
    }

    long size;

    if (fseek(fp, 0, SEEK_END) == -1 ||
        (size = ftell(fp)) < 0)
    {
      fclose(fp);
      return false;
    }

    content.resize(size);
      
    if (fseek(fp, 0, SEEK_SET) == -1)
    {
      fclose(fp);
      return false;
    }

    bool ok = true;

    if (size > 0 &&
        fread(&content[0], size, 1, fp) != 1)
    {
      ok = false;
    }

    fclose(fp);

    return ok;
  }
}
