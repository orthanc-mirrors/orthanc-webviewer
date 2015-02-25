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

#include <vector>
#include <string>

namespace OrthancPlugins
{
  class InstanceInformation
  {
  private:
    bool  hasPosition_;
    float normal_[3];
    float position_[3];
    bool  hasIndex_;
    int   index_;

  public:
    InstanceInformation() : hasPosition_(false), hasIndex_(false)
    {
    }

    InstanceInformation(const std::string& serialized)
    {
      Deserialize(*this, serialized);
    }

    void SetPosition(const std::vector<float>& normal,
                     const std::vector<float>& position);

    void SetIndexInSeries(int index);

    bool HasPosition() const
    {
      return hasPosition_;
    }

    bool HasIndexInSeries() const
    {
      return hasIndex_;
    }

    float GetPosition(size_t i) const;

    float GetNormal(size_t i) const;

    int GetIndexInSeries() const;

    void Serialize(std::string& result) const;

    static void Deserialize(InstanceInformation& result,
                            const std::string& serialized);  
  };
}
