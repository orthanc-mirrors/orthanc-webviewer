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


#pragma once

#include "../Orthanc/Core/ImageFormats/ImageAccessor.h"

#include <orthanc/OrthancCPlugin.h>
#include <stdint.h>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <json/value.h>

namespace OrthancPlugins
{
  class ParsedDicomImage : public boost::noncopyable
  {
  private:
    class PImpl;
    boost::shared_ptr<PImpl> pimpl_;
  
  public:
    ParsedDicomImage(OrthancPluginContext* context,
                     const std::string& instanceId);

    bool GetTag(std::string& result,
                uint16_t group,
                uint16_t element,
                bool stripSpaces = true);

    bool GetCornerstoneMetadata(Json::Value& json);

    bool EncodeUsingDeflate(Json::Value& result,
                            uint8_t compressionLevel  /* between 0 and 9 */);

    bool EncodeUsingJpeg(Json::Value& result,
                         uint8_t quality /* between 0 and 100 */);
  };
}
