/**
 * Orthanc - A Lightweight, RESTful DICOM Store
 * Copyright (C) 2012-2015 Sebastien Jodogne, Medical Physics
 * Department, University Hospital of Liege, Belgium
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * In addition, as a special exception, the copyright holders of this
 * program give permission to link the code of its release with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables. You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL". If you
 * modify file(s) with this exception, you may extend this exception to
 * your version of the file(s), but you are not obligated to do so. If
 * you do not wish to do so, delete this exception statement from your
 * version. If you delete this exception statement from all source files
 * in the program, then also delete it here.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 **/


#pragma once

namespace Orthanc
{
  enum Endianness
  {
    Endianness_Unknown,
    Endianness_Big,
    Endianness_Little
  };

  enum ErrorCode
  {
    // Generic error codes
    ErrorCode_Success,
    ErrorCode_Custom,
    ErrorCode_InternalError,
    ErrorCode_NotImplemented,
    ErrorCode_ParameterOutOfRange,
    ErrorCode_NotEnoughMemory,
    ErrorCode_BadParameterType,
    ErrorCode_BadSequenceOfCalls,
    ErrorCode_InexistentItem,
    ErrorCode_BadRequest,
    ErrorCode_NetworkProtocol,
    ErrorCode_SystemCommand,
    ErrorCode_Database,

    // Specific error codes
    ErrorCode_UriSyntax,
    ErrorCode_InexistentFile,
    ErrorCode_CannotWriteFile,
    ErrorCode_BadFileFormat,
    ErrorCode_Timeout,
    ErrorCode_UnknownResource,
    ErrorCode_IncompatibleDatabaseVersion,
    ErrorCode_FullStorage,
    ErrorCode_CorruptedFile,
    ErrorCode_InexistentTag,
    ErrorCode_ReadOnly,
    ErrorCode_IncompatibleImageFormat,
    ErrorCode_IncompatibleImageSize,
    ErrorCode_SharedLibrary,
    ErrorCode_Plugin
  };

  /**
   * {summary}{The memory layout of the pixels (resp. voxels) of a 2D (resp. 3D) image.}
   **/
  enum PixelFormat
  {
    /**
     * {summary}{Color image in RGB24 format.}
     * {description}{This format describes a color image. The pixels are stored in 3
     * consecutive bytes. The memory layout is RGB.}
     **/
    PixelFormat_RGB24 = 1,

    /**
     * {summary}{Color image in RGBA32 format.}
     * {description}{This format describes a color image. The pixels are stored in 4
     * consecutive bytes. The memory layout is RGBA.}
     **/
    PixelFormat_RGBA32 = 2,

    /**
     * {summary}{Graylevel 8bpp image.}
     * {description}{The image is graylevel. Each pixel is unsigned and stored in one byte.}
     **/
    PixelFormat_Grayscale8 = 3,
      
    /**
     * {summary}{Graylevel, unsigned 16bpp image.}
     * {description}{The image is graylevel. Each pixel is unsigned and stored in two bytes.}
     **/
    PixelFormat_Grayscale16 = 4,
      
    /**
     * {summary}{Graylevel, signed 16bpp image.}
     * {description}{The image is graylevel. Each pixel is signed and stored in two bytes.}
     **/
    PixelFormat_SignedGrayscale16 = 5
  };


  unsigned int GetBytesPerPixel(PixelFormat format);
}
