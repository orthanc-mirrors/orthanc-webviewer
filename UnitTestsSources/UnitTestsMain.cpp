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


#include <gtest/gtest.h>
#include <boost/lexical_cast.hpp>

static int argc_;
static char** argv_;

#include "../Orthanc/OrthancException.h"
#include "../Orthanc/Toolbox.h"
#include "../Orthanc/ImageFormats/ImageBuffer.h"
#include "../Orthanc/ImageFormats/PngWriter.h"
#include "../Plugin/Cache/CacheManager.h"
#include "../Plugin/Cache/CacheScheduler.h"
#include "../Plugin/Cache/ICacheFactory.h"
#include "../Plugin/Cache/ICacheFactory.h"
#include "../Plugin/JpegWriter.h"

using namespace OrthancPlugins;


class CacheManagerTest : public testing::Test
{
private:
  std::auto_ptr<Orthanc::FilesystemStorage>  storage_;
  std::auto_ptr<Orthanc::SQLite::Connection>  db_;
  std::auto_ptr<CacheManager>  cache_;

public:
  virtual void SetUp()
  {
    storage_.reset(new Orthanc::FilesystemStorage("UnitTestsResults"));
    storage_->Clear();
    Orthanc::Toolbox::RemoveFile("UnitTestsResults/cache.db");

    db_.reset(new Orthanc::SQLite::Connection());
    db_->Open("UnitTestsResults/cache.db");

    cache_.reset(new CacheManager(*db_, *storage_));
    cache_->SetSanityCheckEnabled(true);
  }

  virtual void TearDown()
  {
    cache_.reset(NULL);
    db_.reset(NULL);
    storage_.reset(NULL);
  }

  CacheManager& GetCache() 
  {
    return *cache_;
  }

  Orthanc::FilesystemStorage& GetStorage() 
  {
    return *storage_;
  }
};



class TestF : public ICacheFactory
{
private:
  int bundle_;
  
public:
  TestF(int bundle) : bundle_(bundle)
  {
  }

  virtual bool Create(std::string& content,
                      const std::string& key)
  {
    content = "Bundle " + boost::lexical_cast<std::string>(bundle_) + ", item " + key;
    return true;
  }
};


TEST_F(CacheManagerTest, DefaultQuota)
{
  std::set<std::string> f;
  GetStorage().ListAllFiles(f);
  ASSERT_EQ(0, f.size());
  
  GetCache().SetDefaultQuota(10, 0);
  for (int i = 0; i < 30; i++)
  {
    GetStorage().ListAllFiles(f);
    ASSERT_EQ(i >= 10 ? 10 : i, f.size());
    std::string s = boost::lexical_cast<std::string>(i);
    GetCache().Store(0, s, "Test " + s);
  }

  GetStorage().ListAllFiles(f);
  ASSERT_EQ(10, f.size());

  for (int i = 0; i < 30; i++)
  {
    ASSERT_EQ(i >= 20, GetCache().IsCached(0, boost::lexical_cast<std::string>(i)));
  }

  GetCache().SetDefaultQuota(5, 0);
  GetStorage().ListAllFiles(f);
  ASSERT_EQ(5, f.size());
  for (int i = 0; i < 30; i++)
  {
    ASSERT_EQ(i >= 25, GetCache().IsCached(0, boost::lexical_cast<std::string>(i)));
  }

  for (int i = 0; i < 15; i++)
  {
    std::string s = boost::lexical_cast<std::string>(i);
    GetCache().Store(0, s, "Test " + s);
  }

  GetStorage().ListAllFiles(f);
  ASSERT_EQ(5, f.size());

  for (int i = 0; i < 50; i++)
  {
    std::string s = boost::lexical_cast<std::string>(i);
    if (i >= 10 && i < 15)
    {
      std::string tmp;
      ASSERT_TRUE(GetCache().IsCached(0, s));
      ASSERT_TRUE(GetCache().Access(tmp, 0, s));
      ASSERT_EQ("Test " + s, tmp);
    }
    else
    {
      ASSERT_FALSE(GetCache().IsCached(0, s));
    }
  }
}



TEST_F(CacheManagerTest, Invalidate)
{
  GetCache().SetDefaultQuota(10, 0);
  for (int i = 0; i < 30; i++)
  {
    std::string s = boost::lexical_cast<std::string>(i);
    GetCache().Store(0, s, "Test " + s);
  }

  std::set<std::string> f;
  GetStorage().ListAllFiles(f);
  ASSERT_EQ(10, f.size());

  GetCache().Invalidate(0, "25");
  GetStorage().ListAllFiles(f);
  ASSERT_EQ(9, f.size());
  for (int i = 0; i < 50; i++)
  {
    std::string s = boost::lexical_cast<std::string>(i);
    ASSERT_EQ((i >= 20 && i < 30 && i != 25), GetCache().IsCached(0, s));
  }

  for (int i = 0; i < 50; i++)
  {
    GetCache().Invalidate(0, boost::lexical_cast<std::string>(i));
  }

  GetStorage().ListAllFiles(f);
  ASSERT_EQ(0, f.size());
}



TEST(JpegWriter, Basic)
{
  Orthanc::ImageBuffer img(16, 16, Orthanc::PixelFormat_Grayscale8);
  Orthanc::ImageAccessor accessor = img.GetAccessor();
  for (unsigned int y = 0, value = 0; y < img.GetHeight(); y++)
  {
    uint8_t* p = reinterpret_cast<uint8_t*>(accessor.GetRow(y));
    for (unsigned int x = 0; x < img.GetWidth(); x++, p++)
    {
      *p = value++;
    }
  }

  JpegWriter w;
  w.WriteToFile("UnitTestsResults/hello.jpg", accessor);

  std::string s;
  w.WriteToMemory(s, accessor);
  Orthanc::Toolbox::WriteFile(s, "UnitTestsResults/hello2.jpg");
}



int main(int argc, char **argv)
{
  argc_ = argc;
  argv_ = argv;  

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
