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


#include "ParsedDicomImage.h"

#include "../Orthanc/Core/OrthancException.h"
#include "../Orthanc/Core/Toolbox.h"
#include "../Orthanc/Core/Images/ImageProcessing.h"
#include "../Orthanc/Core/Images/ImageBuffer.h"
#include "ViewerToolbox.h"

#include <gdcmImageReader.h>
#include <gdcmImageApplyLookupTable.h>
#include <gdcmImageChangePlanarConfiguration.h>
#include <gdcmImageChangePhotometricInterpretation.h>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/round.hpp>

#include "../Orthanc/Resources/ThirdParty/base64/base64.h"


namespace OrthancPlugins
{
  class ParsedDicomImage::PImpl
  {
  private:
    OrthancPluginContext* context_;
    std::string instanceId_;
    gdcm::ImageReader reader_;
    std::auto_ptr<gdcm::ImageApplyLookupTable> lut_;
    std::auto_ptr<gdcm::ImageChangePhotometricInterpretation> photometric_;
    std::auto_ptr<gdcm::ImageChangePlanarConfiguration> interleaved_;
    std::string decoded_;
    std::auto_ptr<ImageReader> png_;
    bool isDecoded_;

    bool DecodeUsingGdcm()
    {
      // Change photometric interpretation or apply LUT, if required
      {
        const gdcm::Image& image = GetImage();
        if (image.GetPixelFormat().GetSamplesPerPixel() == 1 &&
            image.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::PALETTE_COLOR)
        {
          lut_.reset(new gdcm::ImageApplyLookupTable());
          lut_->SetInput(image);
          if (!lut_->Apply())
          {
            OrthancPluginLogWarning(context_, "GDCM cannot apply the lookup table");
            return false;
          }
        }
        else if (image.GetPixelFormat().GetSamplesPerPixel() == 1)
        {
          if (image.GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::MONOCHROME1 &&
              image.GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::MONOCHROME2)
          {
            photometric_.reset(new gdcm::ImageChangePhotometricInterpretation());
            photometric_->SetInput(image);
            photometric_->SetPhotometricInterpretation(gdcm::PhotometricInterpretation::MONOCHROME2);
            if (!photometric_->Change() ||
                GetImage().GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::MONOCHROME2)
            {
              OrthancPluginLogWarning(context_, "GDCM cannot change the photometric interpretation");
              return false;
            }
          }      
        }
        else 
        {
          if (image.GetPixelFormat().GetSamplesPerPixel() == 3 &&
              image.GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::RGB)
          {
            photometric_.reset(new gdcm::ImageChangePhotometricInterpretation());
            photometric_->SetInput(image);
            photometric_->SetPhotometricInterpretation(gdcm::PhotometricInterpretation::RGB);
            if (!photometric_->Change() ||
                GetImage().GetPhotometricInterpretation() != gdcm::PhotometricInterpretation::RGB)
            {
              OrthancPluginLogWarning(context_, "GDCM cannot change the photometric interpretation");
              return false;
            }
          }
        }
      }

      // Possibly convert planar configuration to interleaved
      {
        const gdcm::Image& image = GetImage();
        if (image.GetPlanarConfiguration() != 0 && 
            image.GetPixelFormat().GetSamplesPerPixel() != 1)
        {
          interleaved_.reset(new gdcm::ImageChangePlanarConfiguration());
          interleaved_->SetInput(image);
          if (!interleaved_->Change() ||
              GetImage().GetPlanarConfiguration() != 0)
          {
            OrthancPluginLogWarning(context_, "GDCM cannot change the planar configuration to interleaved");
            return false;
          }
        }
      }

      // Decode the image to the memory buffer
      {
        const gdcm::Image& image = GetImage();
        decoded_.resize(image.GetBufferLength());
      
        if (decoded_.size() > 0)
        {
          image.GetBuffer(&decoded_[0]);
        }
      }

      return true;
    }


    bool DecodeUsingOrthanc()
    {
      /**
       * This is a DICOM image that cannot be properly decoded by
       * GDCM. Let's give a try with the Orthanc built-in decoder.
       **/
      std::string file = "/instances/" + instanceId_;

      const gdcm::Image& image = GetImage();
      if (image.GetPixelFormat().GetSamplesPerPixel() == 3 ||
          image.GetPixelFormat().GetSamplesPerPixel() == 4)
      {
        file += "/preview";
      }
      else
      {
        file += "/image-uint16";
      }

      std::string png;
      if (!GetStringFromOrthanc(png, context_, file))
      {
        return false;
      }
      else
      {
        try
        {
          png_.reset(new ImageReader(context_, png, OrthancPluginImageFormat_Png));
          return true;
        }
        catch (Orthanc::OrthancException&)
        {
          return false;
        }
      }
    }


    bool Decode()
    {
      if (isDecoded_)
      {
        return true;
      }

      if (DecodeUsingGdcm())
      {
        isDecoded_ = true;
        return true;
      }

      // GDCM cannot decode this image, try and use Orthanc built-in functions
      lut_.reset();
      photometric_.reset();
      interleaved_.reset();
      decoded_.clear();

      if (DecodeUsingOrthanc())
      {
        isDecoded_ = true;
        return true;
      }
      else
      {
        return false;
      }
    }


  public:
    PImpl(OrthancPluginContext* context,
          const std::string& instanceId) : 
      context_(context),
      instanceId_(instanceId),
      isDecoded_(false)
    {
    }


    OrthancPluginContext* GetContext()
    {
      return context_;
    }


    const gdcm::DataSet& GetDataSet() const
    {
      return reader_.GetFile().GetDataSet();
    }


    const gdcm::Image& GetImage() const
    {
      if (interleaved_.get() != NULL)
      {
        return interleaved_->GetOutput();
      }

      if (lut_.get() != NULL)
      {
        return lut_->GetOutput();
      }

      if (photometric_.get() != NULL)
      {
        return photometric_->GetOutput();
      }

      return reader_.GetImage();
    }


    void Parse(const std::string& dicom)
    {
      // Prepare a memory stream over the DICOM instance
      std::stringstream stream(dicom);

      // Parse the DICOM instance using GDCM
      reader_.SetStream(stream);
      if (!reader_.Read())
      {
        OrthancPluginLogError(context_, "GDCM cannot extract an image from this DICOM instance");
        throw Orthanc::OrthancException(Orthanc::ErrorCode_IncompatibleImageFormat);
      }
    }


    bool GetAccessor(Orthanc::ImageAccessor& accessor)
    {
      if (!Decode())
      {
        return false;
      }

      if (png_.get() != NULL)
      {
        // The image was decoded using Orthanc's built-in REST API
        accessor = png_->GetAccessor();
        return true;
      }

      const gdcm::Image& image = GetImage();

      size_t size = decoded_.size();
      void* buffer = (size ? &decoded_[0] : NULL);    
      unsigned int height = image.GetRows();
      unsigned int width = image.GetColumns();

      if (image.GetPixelFormat().GetSamplesPerPixel() == 1 &&
          (image.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME1 ||
           image.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::MONOCHROME2))
      {
        switch (image.GetPixelFormat())
        {
          case gdcm::PixelFormat::UINT16:
            accessor.AssignWritable(Orthanc::PixelFormat_Grayscale16, width, height, 2 * width, buffer);
            return true;

          case gdcm::PixelFormat::INT16:
            accessor.AssignWritable(Orthanc::PixelFormat_SignedGrayscale16, width, height, 2 * width, buffer);
            return true;

          case gdcm::PixelFormat::UINT8:
            accessor.AssignWritable(Orthanc::PixelFormat_Grayscale8, width, height, width, buffer);
            return true;

          default:
            return false;
        }
      }
      else if (image.GetPixelFormat().GetSamplesPerPixel() == 3 &&
               image.GetPhotometricInterpretation() == gdcm::PhotometricInterpretation::RGB)
      {
        switch (image.GetPixelFormat())
        {
          case gdcm::PixelFormat::UINT8:
            accessor.AssignWritable(Orthanc::PixelFormat_RGB24, width, height, 3 * width, buffer);
            return true;

          default:
            return false;
        }      
      }

      return false;
    }
  };


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


  ParsedDicomImage::ParsedDicomImage(OrthancPluginContext* context,
                                     const std::string& instanceId) : 
    pimpl_(new PImpl(context, instanceId))
  {
    std::string file = "/instances/" + instanceId + "/file";

    std::string dicom;
    if (!GetStringFromOrthanc(dicom, context, file))
    {
      throw Orthanc::OrthancException(Orthanc::ErrorCode_UnknownResource);
    }

    pimpl_->Parse(dicom);
  }


  bool ParsedDicomImage::GetTag(std::string& result,
                                uint16_t group,
                                uint16_t element,
                                bool stripSpaces)
  {
    const gdcm::Tag tag(group, element);

    if (pimpl_->GetDataSet().FindDataElement(tag))
    {
      const gdcm::ByteValue* value = pimpl_->GetDataSet().GetDataElement(tag).GetByteValue();
      if (value)
      {
        result = std::string(value->GetPointer(), value->GetLength());

        if (stripSpaces)
        {
          result = Orthanc::Toolbox::StripSpaces(result);
        }

        return true;
      }
    }

    return false;
  }


  bool ParsedDicomImage::GetCornerstoneMetadata(Json::Value& json)
  {
    using namespace Orthanc;

    ImageAccessor accessor;
    if (!pimpl_->GetAccessor(accessor))
    {
      return false;
    }

    float windowCenter, windowWidth;

    switch (accessor.GetFormat())
    {
      case PixelFormat_Grayscale8:
      case PixelFormat_Grayscale16:
      case PixelFormat_SignedGrayscale16:
      {
        int64_t a, b;
        Orthanc::ImageProcessing::GetMinMaxValue(a, b, accessor);
        json["minPixelValue"] = (a < 0 ? static_cast<int32_t>(a) : 0);
        json["maxPixelValue"] = (b > 0 ? static_cast<int32_t>(b) : 1);
        json["color"] = false;
        
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
        json["minPixelValue"] = 0;
        json["maxPixelValue"] = 255;
        json["color"] = true;
        windowCenter = 127.5f;
        windowWidth = 256.0f;
        break;

      default:
        return false;
    }
  
    const gdcm::Image& image = pimpl_->GetImage();
    json["slope"] = image.GetSlope();
    json["intercept"] = image.GetIntercept();
    json["rows"] = image.GetRows();
    json["columns"] = image.GetColumns();
    json["height"] = image.GetRows();
    json["width"] = image.GetColumns();
    json["columnPixelSpacing"] = image.GetSpacing(1);
    json["rowPixelSpacing"] = image.GetSpacing(0);

    json["windowCenter"] = windowCenter * image.GetSlope() + image.GetIntercept();
    json["windowWidth"] = windowWidth * image.GetSlope();

    try
    {
      std::string width, center;
      if (GetTag(center, 0x0028, 0x1050 /*DICOM_TAG_WINDOW_CENTER*/) &&
          GetTag(width, 0x0028, 0x1051 /*DICOM_TAG_WINDOW_WIDTH*/))
      {
        float a = boost::lexical_cast<float>(width);
        float b = boost::lexical_cast<float>(center);
        json["windowWidth"] = a;
        json["windowCenter"] = b;
      }
    }
    catch (boost::bad_lexical_cast&)
    {
    }

    return true;
  }


  bool  ParsedDicomImage::EncodeUsingDeflate(Json::Value& result,
                                             uint8_t compressionLevel  /* between 0 and 9 */)
  {
    using namespace Orthanc;

    ImageAccessor accessor;
    if (!pimpl_->GetAccessor(accessor))
    {
      return false;
    }

    result = Json::objectValue;
    result["Orthanc"] = Json::objectValue;
    if (!GetCornerstoneMetadata(result))
    {
      return false;
    }

    ImageBuffer buffer;
    buffer.SetMinimalPitchForced(true);

    ImageAccessor converted;


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
        ImageProcessing::Convert(converted, accessor);
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
    CompressUsingDeflate(z, pimpl_->GetContext(), converted.GetConstBuffer(), converted.GetSize());

    result["Orthanc"]["PixelData"] = base64_encode(z);  

    return true;
  }



  bool  ParsedDicomImage::EncodeUsingJpeg(Json::Value& result,
                                          uint8_t quality /* between 0 and 100 */)
  {
    using namespace Orthanc;

    ImageAccessor accessor;
    if (!pimpl_->GetAccessor(accessor))
    {
      return false;
    }

    result = Json::objectValue;
    result["Orthanc"] = Json::objectValue;
    GetCornerstoneMetadata(result);

    ImageBuffer buffer;
    buffer.SetMinimalPitchForced(true);

    ImageAccessor converted;

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
    WriteJpegToMemory(jpeg, pimpl_->GetContext(), converted, quality);

    result["Orthanc"]["PixelData"] = base64_encode(jpeg);  
    return true;
  }
}
