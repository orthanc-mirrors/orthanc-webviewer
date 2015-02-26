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


#include <boost/thread.hpp>
#include <boost/lexical_cast.hpp>
#include <EmbeddedResources.h>
#include <boost/filesystem.hpp>

#include "../Orthanc/OrthancException.h"
#include "ViewerToolbox.h"
#include "ViewerPrefetchPolicy.h"
#include "DecodedImageAdapter.h"
#include "InstanceInformationAdapter.h"
#include "SeriesInformationAdapter.h"



class CacheContext
{
private:
  std::auto_ptr<Orthanc::FilesystemStorage>  storage_;
  std::auto_ptr<Orthanc::SQLite::Connection>  db_;
  std::auto_ptr<OrthancPlugins::CacheManager>  cache_;
  std::auto_ptr<OrthancPlugins::CacheScheduler>  scheduler_;

public:
  CacheContext(const std::string& path)
  {
    boost::filesystem::path p(path);

    storage_.reset(new Orthanc::FilesystemStorage(path));
    db_.reset(new Orthanc::SQLite::Connection());
    db_->Open((p / "cache.db").string());

    cache_.reset(new OrthancPlugins::CacheManager(*db_, *storage_));
    //cache_->SetSanityCheckEnabled(true);  // For debug

    scheduler_.reset(new OrthancPlugins::CacheScheduler(*cache_, 100));
  }

  OrthancPlugins::CacheScheduler& GetScheduler()
  {
    return *scheduler_;
  }
};


static OrthancPluginContext* context_ = NULL;
static CacheContext* cache_ = NULL;



static int32_t OnChangeCallback(OrthancPluginChangeType changeType,
                                OrthancPluginResourceType resourceType,
                                const char* resourceId)
{
  try
  {
    if (changeType == OrthancPluginChangeType_NewInstance &&
        resourceType == OrthancPluginResourceType_Instance)
    {
      // On the reception of a new instance, precompute its spatial position
      cache_->GetScheduler().Prefetch(OrthancPlugins::CacheBundle_InstanceInformation, resourceId);
     
      // Indalidate the parent series of the instance
      std::string uri = "/instances/" + std::string(resourceId);
      Json::Value instance;
      if (OrthancPlugins::GetJsonFromOrthanc(instance, context_, uri))
      {
        std::string seriesId = instance["ParentSeries"].asString();
        cache_->GetScheduler().Invalidate(OrthancPlugins::CacheBundle_SeriesInformation, seriesId);
      }
    }

    return 0;
  }
  catch (std::runtime_error& e)
  {
    OrthancPluginLogError(context_, e.what());
    return 0;  // Ignore error
  }
}



template <enum OrthancPlugins::CacheBundle bundle>
int32_t ServeCache(OrthancPluginRestOutput* output,
                   const char* url,
                   const OrthancPluginHttpRequest* request)
{
  try
  {
    if (request->method != OrthancPluginHttpMethod_Get)
    {
      OrthancPluginSendMethodNotAllowed(context_, output, "GET");
      return 0;
    }

    const std::string id = request->groups[0];
    std::string content;

    if (cache_->GetScheduler().Access(content, bundle, id))
    {
      OrthancPluginAnswerBuffer(context_, output, content.c_str(), content.size(), "application/json");
    }
    else
    {
      OrthancPluginSendHttpStatusCode(context_, output, 404);
    }

    return 0;
  }
  catch (Orthanc::OrthancException& e)
  {
    OrthancPluginLogError(context_, e.What());
    return -1;
  }
  catch (std::runtime_error& e)
  {
    OrthancPluginLogError(context_, e.what());
    return -1;
  }
  catch (boost::bad_lexical_cast&)
  {
    OrthancPluginLogError(context_, "Bad lexical cast");
    return -1;
  }
}




#if ORTHANC_STANDALONE == 0
static int32_t ServeWebViewer(OrthancPluginRestOutput* output,
                              const char* url,
                              const OrthancPluginHttpRequest* request)
{
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context_, output, "GET");
    return 0;
  }

  const std::string path = std::string(WEB_VIEWER_PATH) + std::string(request->groups[0]);
  const char* mime = OrthancPlugins::GetMimeType(path);

  std::string s;
  if (OrthancPlugins::ReadFile(s, path))
  {
    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context_, output, resource, s.size(), mime);
  }
  else
  {
    std::string s = "Inexistent file in served folder: " + path;
    OrthancPluginLogError(context_, s.c_str());
    OrthancPluginSendHttpStatusCode(context_, output, 404);
  }

  return 0;
}
#endif



template <enum OrthancPlugins::EmbeddedResources::DirectoryResourceId folder>
static int32_t ServeEmbeddedFolder(OrthancPluginRestOutput* output,
                                   const char* url,
                                   const OrthancPluginHttpRequest* request)
{
  if (request->method != OrthancPluginHttpMethod_Get)
  {
    OrthancPluginSendMethodNotAllowed(context_, output, "GET");
    return 0;
  }

  std::string path = "/" + std::string(request->groups[0]);
  const char* mime = OrthancPlugins::GetMimeType(path);

  try
  {
    std::string s;
    OrthancPlugins::EmbeddedResources::GetDirectoryResource(s, folder, path.c_str());

    const char* resource = s.size() ? s.c_str() : NULL;
    OrthancPluginAnswerBuffer(context_, output, resource, s.size(), mime);

    return 0;
  }
  catch (std::runtime_error&)
  {
    std::string s = "Unknown static resource in plugin: " + std::string(request->groups[0]);
    OrthancPluginLogError(context_, s.c_str());
    OrthancPluginSendHttpStatusCode(context_, output, 404);
    return 0;
  }
}




extern "C"
{
  ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext* context)
  {
    using namespace OrthancPlugins;

    context_ = context;
    OrthancPluginLogWarning(context_, "Initializing the Web viewer");


    /* Check the version of the Orthanc core */
    if (OrthancPluginCheckVersion(context_) == 0)
    {
      char info[1024];
      sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
              context_->orthancVersion,
              ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
              ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
      OrthancPluginLogError(context_, info);
      return -1;
    }

    OrthancPluginSetDescription(context_, "Provides a Web viewer of DICOM series within Orthanc.");


    /* By default, use half of the available processing cores for the decoding of DICOM images */
    int decodingThreads = boost::thread::hardware_concurrency() / 2;
    if (decodingThreads == 0)
    {
      decodingThreads = 1;
    }


    try
    {
      /* Read the configuration of the Web viewer */
      Json::Value configuration;
      if (!ReadConfiguration(configuration, context))
      {
        OrthancPluginLogError(context_, "Unable to read the configuration file of Orthanc");
        return -1;
      }

      // By default, the cache of the Web viewer is located inside the
      // "StorageDirectory" of Orthanc
      boost::filesystem::path cachePath = GetStringValue(configuration, "StorageDirectory", ".");
      cachePath /= "WebViewerCache";
      int cacheSize = 100;  // By default, a cache of 100 MB is used

      if (configuration.isMember("WebViewer"))
      {
        std::string key = "CachePath";
        if (!configuration["WebViewer"].isMember(key))
        {
          // For backward compatibility with the initial release of the Web viewer
          key = "Cache";
        }

        cachePath = GetStringValue(configuration["WebViewer"], key, cachePath.string());
        cacheSize = GetIntegerValue(configuration["WebViewer"], "CacheSize", cacheSize);
        decodingThreads = GetIntegerValue(configuration["WebViewer"], "Threads", decodingThreads);
      }

      std::string message = ("Web viewer using " + boost::lexical_cast<std::string>(decodingThreads) + 
                             " threads for the decoding of the DICOM images");
      OrthancPluginLogWarning(context_, message.c_str());

      if (decodingThreads <= 0 ||
          cacheSize <= 0)
      {
        throw Orthanc::OrthancException(Orthanc::ErrorCode_ParameterOutOfRange);
      }

      message = "Storing the cache of the Web viewer in folder: " + cachePath.string();
      OrthancPluginLogWarning(context_, message.c_str());

   
      /* Create the cache */
      cache_ = new CacheContext(cachePath.string());
      cache_->GetScheduler().RegisterPolicy(new ViewerPrefetchPolicy(context_));
      cache_->GetScheduler().Register(CacheBundle_SeriesInformation, 
                                      new SeriesInformationAdapter(context_, cache_->GetScheduler()), 1);
      cache_->GetScheduler().Register(CacheBundle_InstanceInformation, 
                                      new InstanceInformationAdapter(context_), 1);
      cache_->GetScheduler().Register(CacheBundle_DecodedImage, 
                                      new DecodedImageAdapter(context_), decodingThreads);


      /* Set the quotas */
      cache_->GetScheduler().SetQuota(CacheBundle_SeriesInformation, 1000, 0);    // Keep info about 1000 series
      cache_->GetScheduler().SetQuota(CacheBundle_InstanceInformation, 10000, 0); // Keep info about 10,000 instances
      
      message = "Web viewer using a cache of " + boost::lexical_cast<std::string>(cacheSize) + " MB";
      OrthancPluginLogWarning(context_, message.c_str());

      cache_->GetScheduler().SetQuota(CacheBundle_DecodedImage, 0, static_cast<uint64_t>(cacheSize) * 1024 * 1024);
    }
    catch (std::runtime_error& e)
    {
      OrthancPluginLogError(context_, e.what());
      return -1;
    }
    catch (Orthanc::OrthancException& e)
    {
      OrthancPluginLogError(context_, e.What());
      return -1;
    }


    /* Install the callbacks */
    OrthancPluginRegisterRestCallback(context_, "/web-viewer/series/(.*)", ServeCache<CacheBundle_SeriesInformation>);
    OrthancPluginRegisterRestCallback(context_, "/web-viewer/instances/(.*)", ServeCache<CacheBundle_DecodedImage>);
    OrthancPluginRegisterRestCallback(context, "/web-viewer/libs/(.*)", ServeEmbeddedFolder<EmbeddedResources::JAVASCRIPT_LIBS>);

#if ORTHANC_STANDALONE == 1
    OrthancPluginRegisterRestCallback(context, "/web-viewer/app/(.*)", ServeEmbeddedFolder<EmbeddedResources::WEB_VIEWER>);
#else
    OrthancPluginRegisterRestCallback(context, "/web-viewer/app/(.*)", ServeWebViewer);
#endif

    OrthancPluginRegisterOnChangeCallback(context, OnChangeCallback);


    /* Extend the default Orthanc Explorer with custom JavaScript */
    std::string explorer;
    EmbeddedResources::GetFileResource(explorer, EmbeddedResources::ORTHANC_EXPLORER);
    OrthancPluginExtendOrthancExplorer(context_, explorer.c_str());

    return 0;
  }


  ORTHANC_PLUGINS_API void OrthancPluginFinalize()
  {
    OrthancPluginLogWarning(context_, "Finalizing the Web viewer");

    if (cache_ != NULL)
    {
      delete cache_;
      cache_ = NULL;
    }
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetName()
  {
    return "web-viewer";
  }


  ORTHANC_PLUGINS_API const char* OrthancPluginGetVersion()
  {
    return "1.0";
  }
}
