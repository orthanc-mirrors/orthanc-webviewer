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


#include "InstanceInformation.h"

#include "../Orthanc/OrthancException.h"

#include <cassert>
#include <string.h>
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>


namespace OrthancPlugins
{
  void InstanceInformation::SetPosition(const std::vector<float>& normal,
                                        const std::vector<float>& position)
  {
    if (normal.size() != 3 ||
        position.size() != 3)
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_BadParameterType);
    }

    hasPosition_ = true;
    memcpy(normal_, &normal[0], sizeof(float) * 3);
    memcpy(position_, &position[0], sizeof(float) * 3);
  }

  void InstanceInformation::SetIndexInSeries(int index)
  {
    hasIndex_ = true;
    index_ = index;
  }

  float InstanceInformation::GetPosition(size_t i) const
  {
    assert(hasPosition_ && i < 3);
    return position_[i];
  }

  float InstanceInformation::GetNormal(size_t i) const
  {
    assert(hasPosition_ && i < 3);
    return normal_[i];
  }

  int InstanceInformation::GetIndexInSeries() const
  {
    assert(hasIndex_);
    return index_;
  }

  void InstanceInformation::Serialize(std::string& result) const
  {
    Json::Value value = Json::objectValue;

    if (hasPosition_)
    {
      value["Normal"] = Json::arrayValue;
      value["Normal"][0] = normal_[0];
      value["Normal"][1] = normal_[1];
      value["Normal"][2] = normal_[2];

      value["Position"] = Json::arrayValue;
      value["Position"][0] = position_[0];
      value["Position"][1] = position_[1];
      value["Position"][2] = position_[2];
    }

    if (hasIndex_)
    {
      value["Index"] = index_;
    }

    Json::FastWriter writer;
    result = writer.write(value);
  }


  void InstanceInformation::Deserialize(InstanceInformation& result,
                                        const std::string& serialized)
  {
    result = InstanceInformation();

    Json::Reader reader;
    Json::Value value;

    if (!reader.parse(serialized, value) ||
        value.type() != Json::objectValue)
    {
      return;
    }

    if (value.isMember("Normal"))
    {
      std::vector<float> normal(3), position(3);
      normal[0] = value["Normal"][0].asFloat();
      normal[1] = value["Normal"][1].asFloat();
      normal[2] = value["Normal"][2].asFloat();
      position[0] = value["Position"][0].asFloat();
      position[1] = value["Position"][1].asFloat();
      position[2] = value["Position"][2].asFloat();
      result.SetPosition(normal, position);
    }

    if (value.isMember("Index"))
    {
      result.SetIndexInSeries(value["Index"].asInt());
    }
  }
}
