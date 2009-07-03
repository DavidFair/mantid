#ifndef MANTID_CONFIGSERVICETEST_H_
#define MANTID_CONFIGSERVICETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Logger.h"
#include "Poco/Path.h"
#include "boost/shared_ptr.hpp"
#include "TestChannel.hh"
#include <string>

using namespace Mantid::Kernel;

class ConfigServiceTest : public CxxTest::TestSuite
{
public: 

  ConfigServiceTest()
  {
	  ConfigService::Instance().loadConfig("MantidTest.properties");
  }

  void testLogging()
  {
	  //attempt some logging
	  Logger& log1 = Logger::get("logTest");

	  TS_ASSERT_THROWS_NOTHING(log1.debug("a debug string"));
	  TS_ASSERT_THROWS_NOTHING(log1.information("an information string"));
	  TS_ASSERT_THROWS_NOTHING(log1.information("a notice string"));
	  TS_ASSERT_THROWS_NOTHING(log1.warning("a warning string"));
	  TS_ASSERT_THROWS_NOTHING(log1.error("an error string"));
	  TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string"));

	  TS_ASSERT_THROWS_NOTHING(
		log1.fatal()<<"A fatal message from the stream operators " << 4.5 << std::endl;
		log1.error()<<"A error message from the stream operators " << -0.2 << std::endl;
		log1.warning()<<"A warning message from the stream operators " << 999.99 << std::endl;
		log1.notice()<<"A notice message from the stream operators " << 0.0 << std::endl;
		log1.information()<<"A information message from the stream operators " << -999.99 << std::endl;
		log1.debug()<<"A debug message from the stream operators " << 5684568 << std::endl;


	  );

	  //checking the level - this should be set to debug in the config file
	  //therefore this should only return false for debug
	  TS_ASSERT(log1.is(Logger::PRIO_DEBUG) == false); //debug
	  TS_ASSERT(log1.is(Logger::PRIO_INFORMATION)); //information
	  TS_ASSERT(log1.is(Logger::PRIO_NOTICE)); //information
	  TS_ASSERT(log1.is(Logger::PRIO_WARNING)); //warning
	  TS_ASSERT(log1.is(Logger::PRIO_ERROR)); //error
	  TS_ASSERT(log1.is(Logger::PRIO_FATAL)); //fatal
  }

	void testEnabled()
  {
	  //attempt some logging
	  Logger& log1 = Logger::get("logTestEnabled");
		TS_ASSERT(log1.getEnabled());
	  TS_ASSERT_THROWS_NOTHING(log1.fatal("a fatal string with enabled=true"));
		TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"A fatal message from the stream operators with enabled=true " << 4.5 << std::endl;);
		
		TS_ASSERT_THROWS_NOTHING(log1.setEnabled(false));
		TS_ASSERT(!log1.getEnabled());
		TS_ASSERT_THROWS_NOTHING(log1.fatal("YOU SHOULD NEVER SEE THIS"));
		TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"YOU SHOULD NEVER SEE THIS VIA A STREAM" << std::endl;);
		
		TS_ASSERT_THROWS_NOTHING(log1.setEnabled(true));
		TS_ASSERT(log1.getEnabled());
		TS_ASSERT_THROWS_NOTHING(log1.fatal("you are allowed to see this"));
		TS_ASSERT_THROWS_NOTHING(log1.fatal()<<"you are allowed to see this via a stream" << std::endl;);

  }

	void testChangeName()
  {
	  //attempt some logging
	  Logger& log1 = Logger::get("logTestName1");
	  TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName1"));
		TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName1 via a stream" << std::endl;);
		
		TS_ASSERT_THROWS_NOTHING(log1.setName("logTestName2"));
		TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName2"));
		TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName2 via a stream" << std::endl;);
		
		TS_ASSERT_THROWS_NOTHING(log1.setName("logTestName1"));
		TS_ASSERT_THROWS_NOTHING(log1.error("This should be from logTestName1"));
		TS_ASSERT_THROWS_NOTHING(log1.error()<<"This should be from logTestName1 via a stream" << std::endl;);
		
  }

  void TestSystemValues()
  {
	  //we cannot test the return values here as they will differ based on the environment.
	  //therfore we will just check they return a non empty string.
	  std::string osName = ConfigService::Instance().getOSName();
	  TS_ASSERT_LESS_THAN(0, osName.length()); //check that the string is not empty
	  std::string osArch = ConfigService::Instance().getOSArchitecture();
	  TS_ASSERT_LESS_THAN(0, osArch.length()); //check that the string is not empty
	  std::string osCompName = ConfigService::Instance().getComputerName();
	  TS_ASSERT_LESS_THAN(0, osCompName.length()); //check that the string is not empty
	  TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getOSVersion().length()); //check that the string is not empty
	  TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getCurrentDir().length()); //check that the string is not empty
//	  TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getHomeDir().length()); //check that the string is not empty
	  TS_ASSERT_LESS_THAN(0, ConfigService::Instance().getTempDir().length()); //check that the string is not empty
  }

  void TestCustomProperty()
  {
	  //Mantid.legs is defined in the properties script as 6
	  std::string legCountString = ConfigService::Instance().getString("mantid.legs");
	  TS_ASSERT_EQUALS(legCountString, "6");
  }

   void TestCustomPropertyAsValue()
  {
	  //Mantid.legs is defined in the properties script as 6
	  int value = 0;
	  int retVal = ConfigService::Instance().getValue("mantid.legs",value);
	  double dblValue = 0;
	  retVal = ConfigService::Instance().getValue("mantid.legs",dblValue);

	  TS_ASSERT_EQUALS(value, 6);
	  TS_ASSERT_EQUALS(dblValue, 6.0);
  }

  void TestMissingProperty()
  {
	  //Mantid.noses is not defined in the properties script 
	  std::string noseCountString = ConfigService::Instance().getString("mantid.noses");
    //this should return an empty string

	  TS_ASSERT_EQUALS(noseCountString, "");
  }

  void TestRelativeToAbsolute()
  {
    std::string instrumentPath = ConfigService::Instance().getString("instrumentDefinition.directory");
    TS_ASSERT( Poco::Path(instrumentPath).isAbsolute() );
  } 

	void TestAppendProperties()
	{

		//This should clear out all old properties
		ConfigService::Instance().loadConfig("MantidTest.properties");
		//this should return an empty string
		TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

		//This should append a new properties file properties
		ConfigService::Instance().loadConfig("MantidTest.user.properties",true);
		//this should now be valid
		TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "5");
    //this should have been overridden
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "76");
		//this should have been left alone
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

		//This should clear out all old properties
		ConfigService::Instance().loadConfig("MantidTest.properties");
		//this should return an empty string
		TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.noses"), "");
    //this should pass
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.legs"), "6");
	  TS_ASSERT_EQUALS(ConfigService::Instance().getString("mantid.thorax"), "1");

	}

};

#endif /*MANTID_CONFIGSERVICETEST_H_*/
