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


#include "SeriesInformationAdapter.h"

#include "ViewerToolbox.h"
#include "SeriesVolumeSorter.h"

#include "../Orthanc/Core/OrthancException.h"

namespace OrthancPlugins
{
  bool SeriesInformationAdapter::Create(std::string& content,
                                        const std::string& seriesId)
  {
    std::string message = "Ordering instances of series: " + seriesId;
    OrthancPluginLogInfo(context_, message.c_str());

    Json::Value series, study, patient;
    if (!GetJsonFromOrthanc(series, context_, "/series/" + seriesId) ||
        !GetJsonFromOrthanc(study, context_, "/studies/" + series["ID"].asString() + "/module?simplify") ||
        !GetJsonFromOrthanc(patient, context_, "/studies/" + series["ID"].asString() + "/module-patient?simplify") ||
        !series.isMember("Instances") ||
        series["Instances"].type() != Json::arrayValue)
    {
      return false;
    }

    Json::Value result;
    result["ID"] = seriesId;
    result["SeriesDescription"] = series["MainDicomTags"]["SeriesDescription"].asString();
    result["StudyDescription"] = study["StudyDescription"].asString();
    result["PatientID"] = patient["PatientID"].asString();
    result["PatientName"] = patient["PatientName"].asString();
    result["SortedInstances"] = Json::arrayValue;

    SeriesVolumeSorter sorter;
    sorter.Reserve(series["Instances"].size());

    for (Json::Value::ArrayIndex i = 0; i < series["Instances"].size(); i++)
    {
      const std::string instanceId = series["Instances"][i].asString();
      std::string tmp;

      if (!cache_.Access(tmp, CacheBundle_InstanceInformation, instanceId))
      {
        OrthancPluginLogError(context_, "The cache is corrupted. Delete it to reconstruct it.");
        throw Orthanc::OrthancException(Orthanc::ErrorCode_CorruptedFile);
      }

      InstanceInformation instance(tmp);
      sorter.AddInstance(instanceId, instance);
    }

    for (size_t i = 0; i < sorter.GetSize(); i++)
    {
      result["SortedInstances"].append(sorter.GetInstance(i));
    }

    content = result.toStyledString();

    return true;
  }
}
