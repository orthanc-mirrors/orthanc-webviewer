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

#include "InstanceInformation.h"

#include <json/reader.h>
#include <boost/noncopyable.hpp>

namespace OrthancPlugins
{
  class SeriesVolumeSorter : public boost::noncopyable
  {
  private:
    typedef std::pair<std::string, float>  InstanceWithPosition;
    typedef std::pair<std::string, int>  InstanceWithIndex;

    static bool ComparePosition(const InstanceWithPosition& a,
                                const InstanceWithPosition& b)
    {
      return a.second < b.second;
    }

    static bool CompareIndex(const InstanceWithIndex& a,
                             const InstanceWithIndex& b)
    {
      return a.second < b.second;
    }  

    bool            sorted_;
    bool            isVolume_;
    float           normal_[3];

    std::vector<InstanceWithPosition>  positions_;
    std::vector<InstanceWithIndex>  indexes_;

  public:
    SeriesVolumeSorter();

    void Reserve(size_t countInstances);

    void AddInstance(const std::string& instanceId,
                     const InstanceInformation& instance);

    size_t GetSize() const
    {
      return indexes_.size();
    }

    std::string  GetInstance(size_t index); 
  };
}
