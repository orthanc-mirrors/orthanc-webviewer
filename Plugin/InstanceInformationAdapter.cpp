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


#include "InstanceInformationAdapter.h"

#include "ViewerToolbox.h"
#include "InstanceInformation.h"

#include <json/value.h>

static const char* IMAGE_ORIENTATION_PATIENT = "ImageOrientationPatient";
static const char* IMAGE_POSITION_PATIENT = "ImagePositionPatient";
static const char* INDEX_IN_SERIES = "IndexInSeries";


namespace OrthancPlugins
{
  bool InstanceInformationAdapter::Create(std::string& content,
                                          const std::string& instanceId)
  {
    std::string message = "Creating spatial information for instance: " + instanceId;
    OrthancPluginLogInfo(context_, message.c_str());

    std::string uri = "/instances/" + instanceId;

    Json::Value instance, tags;
    if (!GetJsonFromOrthanc(instance, context_, uri) ||
        !GetJsonFromOrthanc(tags, context_, uri + "/tags?simplify") ||
        instance.type() != Json::objectValue ||
        tags.type() != Json::objectValue)
    {
      return false;
    }

    InstanceInformation info;

    if (tags.isMember(IMAGE_ORIENTATION_PATIENT) &&
        tags.isMember(IMAGE_POSITION_PATIENT) &&
        tags[IMAGE_ORIENTATION_PATIENT].type() == Json::stringValue &&
        tags[IMAGE_POSITION_PATIENT].type() == Json::stringValue)
    {
      std::vector<float> cosines, position;
      if (TokenizeVector(cosines, tags[IMAGE_ORIENTATION_PATIENT].asString(), 6) &&
          TokenizeVector(position, tags[IMAGE_POSITION_PATIENT].asString(), 3))
      {
        std::vector<float> normal(3);
        normal[0] = cosines[1] * cosines[5] - cosines[2] * cosines[4];
        normal[1] = cosines[2] * cosines[3] - cosines[0] * cosines[5];
        normal[2] = cosines[0] * cosines[4] - cosines[1] * cosines[3];

        info.SetPosition(normal, position);
      }
    }

    if (instance.isMember(INDEX_IN_SERIES) &&
        instance[INDEX_IN_SERIES].type() == Json::intValue)
    {
      info.SetIndexInSeries(instance[INDEX_IN_SERIES].asInt());
    }

    info.Serialize(content);
    return true;
  }
}
