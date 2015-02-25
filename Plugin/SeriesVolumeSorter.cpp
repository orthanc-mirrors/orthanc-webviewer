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


#include "SeriesVolumeSorter.h"

#include <algorithm>
#include <limits>
#include <math.h>
#include <cassert>

namespace OrthancPlugins
{
  SeriesVolumeSorter::SeriesVolumeSorter() : 
    isVolume_(true), 
    sorted_(true)
  {
  }


  void SeriesVolumeSorter::Reserve(size_t countInstances)
  {
    positions_.reserve(countInstances);
    indexes_.reserve(countInstances);
  }


  void SeriesVolumeSorter::AddInstance(const std::string& instanceId,
                                       const InstanceInformation& instance)
  {
    if (instance.HasIndexInSeries())
    {
      indexes_.push_back(std::make_pair(instanceId, instance.GetIndexInSeries()));
    }

    if (!isVolume_ ||
        !instance.HasPosition())
    {
      isVolume_ = false;
    }
    else
    {
      if (positions_.size() == 0)
      {
        // This is the first slice in a possible 3D volume. Remember its normal.
        normal_[0] = instance.GetNormal(0);
        normal_[1] = instance.GetNormal(1);
        normal_[2] = instance.GetNormal(2);
      }
      else
      {
        static const float THRESHOLD = 10.0f * std::numeric_limits<float>::epsilon();

        // This is still a possible 3D volume. Check whether the normal
        // is constant wrt. the previous slices.
        if (fabs(normal_[0] - instance.GetNormal(0)) > THRESHOLD ||
            fabs(normal_[1] - instance.GetNormal(1)) > THRESHOLD ||
            fabs(normal_[2] - instance.GetNormal(2)) > THRESHOLD)
        {
          // The normal is not constant, not a 3D volume.
          isVolume_ = false;
          positions_.clear();
        }
      }

      if (isVolume_)
      {
        float distance = (normal_[0] * instance.GetPosition(0) + 
                          normal_[1] * instance.GetPosition(1) +
                          normal_[2] * instance.GetPosition(2));
        positions_.push_back(std::make_pair(instanceId, distance));
      }
    }

    sorted_ = false;
  }


  std::string  SeriesVolumeSorter::GetInstance(size_t index)
  {
    if (!sorted_)
    {
      if (isVolume_)
      {
        assert(indexes_.size() == positions_.size());
        std::sort(positions_.begin(), positions_.end(), ComparePosition);

        float a = positions_.front().second;
        float b = positions_.back().second;
        assert(a <= b);
        
        if (fabs(b - a) <= 10.0f * std::numeric_limits<float>::epsilon())
        {
          // Not enough difference between the minimum and maximum
          // positions along the normal of the volume
          isVolume_ = false;
        }
      }

      if (!isVolume_)
      {
        std::sort(indexes_.begin(), indexes_.end(), CompareIndex);
      }
    }

    if (isVolume_)
    {
      return positions_[index].first;
    }
    else
    {
      return indexes_[index].first;
    }
  }
}
