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

#include "../Orthanc/Core/Images/ImageBuffer.h"
#include "../Orthanc/Core/Images/ImageProcessing.h"
#include "../Orthanc/Core/OrthancException.h"
#include "../Orthanc/Plugins/Samples/GdcmDecoder/OrthancImageWrapper.h"
#include "../Orthanc/Resources/ThirdParty/base64/base64.h"
#include "ViewerToolbox.h"

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


    bool ok = false;

    Json::Value tags;
    std::string dicom;
    if (!GetStringFromOrthanc(dicom, context_, "/instances/" + instanceId + "/file") ||
        !GetJsonFromOrthanc(tags, context_, "/instances/" + instanceId + "/tags"))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }

    std::auto_ptr<OrthancImageWrapper> image(decoderCache_.Decode(context_, dicom.c_str(), dicom.size(), 0 /* TODO Frame */));

    Json::Value json;
    if (GetCornerstoneMetadata(json, tags, *image))
    {
      if (type == CompressionType_Deflate)
      {
        ok = EncodeUsingDeflate(json, *image, 9);
      }
      else if (type == CompressionType_Jpeg)
      {
        ok = EncodeUsingJpeg(json, *image, level);
      }
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


  static bool GetTagValue(std::string& value,
                          const Json::Value& tags,
                          const std::string& tag)
  {
    if (tags.type() == Json::objectValue &&
        tags.isMember(tag) &&
        tags[tag].type() == Json::objectValue &&
        tags[tag].isMember("Type") &&
        tags[tag].isMember("Value") &&
        tags[tag]["Type"].type() == Json::stringValue &&
        tags[tag]["Value"].type() == Json::stringValue &&
        tags[tag]["Type"].asString() == "String")
    {
      value = tags[tag]["Value"].asString();
      return true;
    }        
    else
    {
      return false;
    }
  }
                                 


  bool DecodedImageAdapter::GetCornerstoneMetadata(Json::Value& result,
                                                   const Json::Value& tags,
                                                   OrthancImageWrapper& image)
  {
    using namespace Orthanc;

    float windowCenter, windowWidth;

    Orthanc::ImageAccessor accessor;
    accessor.AssignReadOnly(OrthancPlugins::Convert(image.GetFormat()), image.GetWidth(),
                            image.GetHeight(), image.GetPitch(), image.GetBuffer());

    switch (accessor.GetFormat())
    {
      case PixelFormat_Grayscale8:
      case PixelFormat_Grayscale16:
      case PixelFormat_SignedGrayscale16:
      {
        int64_t a, b;
        Orthanc::ImageProcessing::GetMinMaxValue(a, b, accessor);
        result["minPixelValue"] = (a < 0 ? static_cast<int32_t>(a) : 0);
        result["maxPixelValue"] = (b > 0 ? static_cast<int32_t>(b) : 1);
        result["color"] = false;
        
        windowCenter = static_cast<float>(a + b) / 2.0f;
        
        if (a == b)
        {
          windowWidth = 256.0f;  // Arbitrary value
        }
        else
        {
          windowWidth = static_cast<float>(b - a) / 2.0f;
        }

        break;
      }

      case PixelFormat_RGB24:
        result["minPixelValue"] = 0;
        result["maxPixelValue"] = 255;
        result["color"] = true;
        windowCenter = 127.5f;
        windowWidth = 256.0f;
        break;

      default:
        return false;
    }

    result["slope"] = image.GetSlope();
    result["intercept"] = image.GetIntercept();
    result["rows"] = image.GetHeight();
    result["columns"] = image.GetWidth();
    result["height"] = image.GetHeight();
    result["width"] = image.GetWidth();
    result["columnPixelSpacing"] = image.GetColumnPixelSpacing();
    result["rowPixelSpacing"] = image.GetRowPixelSpacing();

    result["windowCenter"] = windowCenter * image.GetSlope() + image.GetIntercept();
    result["windowWidth"] = windowWidth * image.GetSlope();

    try
    {
      std::string width, center;
      if (GetTagValue(center, tags, "0028,1050" /*DICOM_TAG_WINDOW_CENTER*/) &&
          GetTagValue(width, tags, "0028,1051" /*DICOM_TAG_WINDOW_WIDTH*/))
      {
        float a = boost::lexical_cast<float>(width);
        float b = boost::lexical_cast<float>(center);
        result["windowWidth"] = a;
        result["windowCenter"] = b;
      }
    }
    catch (boost::bad_lexical_cast&)
    {
    }

    return true;
  }



  bool  DecodedImageAdapter::EncodeUsingDeflate(Json::Value& result,
                                                OrthancImageWrapper& image,
                                                uint8_t compressionLevel  /* between 0 and 9 */)
  {
    Orthanc::ImageAccessor accessor;
    accessor.AssignReadOnly(OrthancPlugins::Convert(image.GetFormat()), image.GetWidth(),
                            image.GetHeight(), image.GetPitch(), image.GetBuffer());

    Orthanc::ImageBuffer buffer;
    buffer.SetMinimalPitchForced(true);

    Orthanc::ImageAccessor converted;

    switch (accessor.GetFormat())
    {
      case Orthanc::PixelFormat_RGB24:
        converted = accessor;
        break;

      case Orthanc::PixelFormat_Grayscale8:
      case Orthanc::PixelFormat_Grayscale16:
        buffer.SetFormat(Orthanc::PixelFormat_SignedGrayscale16);
        buffer.SetWidth(accessor.GetWidth());
        buffer.SetHeight(accessor.GetHeight());
        converted = buffer.GetAccessor();
        Orthanc::ImageProcessing::Convert(converted, accessor);
        break;

      case Orthanc::PixelFormat_SignedGrayscale16:
        converted = accessor;
        break;

      default:
        // Unsupported pixel format
        return false;
    }

    // Sanity check: The pitch must be minimal
    assert(converted.GetSize() == converted.GetWidth() * converted.GetHeight() * 
           GetBytesPerPixel(converted.GetFormat()));
    result["Orthanc"]["Compression"] = "Deflate";
    result["sizeInBytes"] = converted.GetSize();

    std::string z;
    CompressUsingDeflate(z, image.GetContext(), converted.GetConstBuffer(), converted.GetSize());

    result["Orthanc"]["PixelData"] = base64_encode(z);  

    return true;
  }



  template <typename TargetType, typename SourceType>
  static void ChangeDynamics(Orthanc::ImageAccessor& target,
                             const Orthanc::ImageAccessor& source,
                             SourceType source1, TargetType target1,
                             SourceType source2, TargetType target2)
  {
    if (source.GetWidth() != target.GetWidth() ||
        source.GetHeight() != target.GetHeight())
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageSize);
    }

    float scale = static_cast<float>(target2 - target1) / static_cast<float>(source2 - source1);
    float offset = static_cast<float>(target1) - scale * static_cast<float>(source1);

    const float minValue = static_cast<float>(std::numeric_limits<TargetType>::min());
    const float maxValue = static_cast<float>(std::numeric_limits<TargetType>::max());

    for (unsigned int y = 0; y < source.GetHeight(); y++)
    {
      const SourceType* p = reinterpret_cast<const SourceType*>(source.GetConstRow(y));
      TargetType* q = reinterpret_cast<TargetType*>(target.GetRow(y));

      for (unsigned int x = 0; x < source.GetWidth(); x++, p++, q++)
      {
        float v = (scale * static_cast<float>(*p)) + offset;

        if (v > maxValue)
        {
          *q = std::numeric_limits<TargetType>::max();
        }
        else if (v < minValue)
        {
          *q = std::numeric_limits<TargetType>::min();
        }
        else
        {
          *q = static_cast<TargetType>(boost::math::iround(v));
        }
      }
    }
  }


  bool  DecodedImageAdapter::EncodeUsingJpeg(Json::Value& result,
                                             OrthancImageWrapper& image,
                                             uint8_t quality /* between 0 and 100 */)
  {
    Orthanc::ImageAccessor accessor;
    accessor.AssignReadOnly(OrthancPlugins::Convert(image.GetFormat()), image.GetWidth(),
                            image.GetHeight(), image.GetPitch(), image.GetBuffer());

    Orthanc::ImageBuffer buffer;
    buffer.SetMinimalPitchForced(true);

    Orthanc::ImageAccessor converted;

    if (accessor.GetFormat() == Orthanc::PixelFormat_Grayscale8 ||
        accessor.GetFormat() == Orthanc::PixelFormat_RGB24)
    {
      result["Orthanc"]["Stretched"] = false;
      converted = accessor;
    }
    else if (accessor.GetFormat() == Orthanc::PixelFormat_Grayscale16 ||
             accessor.GetFormat() == Orthanc::PixelFormat_SignedGrayscale16)
    {
      result["Orthanc"]["Stretched"] = true;
      buffer.SetFormat(Orthanc::PixelFormat_Grayscale8);
      buffer.SetWidth(accessor.GetWidth());
      buffer.SetHeight(accessor.GetHeight());
      converted = buffer.GetAccessor();

      int64_t a, b;
      Orthanc::ImageProcessing::GetMinMaxValue(a, b, accessor);
      result["Orthanc"]["StretchLow"] = static_cast<int32_t>(a);
      result["Orthanc"]["StretchHigh"] = static_cast<int32_t>(b);

      if (accessor.GetFormat() == Orthanc::PixelFormat_Grayscale16)
      {
        ChangeDynamics<uint8_t, uint16_t>(converted, accessor, a, 0, b, 255);
      }
      else
      {
        ChangeDynamics<uint8_t, int16_t>(converted, accessor, a, 0, b, 255);
      }
    }
    else
    {
      return false;
    }
    
    result["Orthanc"]["Compression"] = "Jpeg";
    result["sizeInBytes"] = converted.GetSize();

    std::string jpeg;
    WriteJpegToMemory(jpeg, image.GetContext(), converted, quality);

    result["Orthanc"]["PixelData"] = base64_encode(jpeg);  
    return true;
  }
}
