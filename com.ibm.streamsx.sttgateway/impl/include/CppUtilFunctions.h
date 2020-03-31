/*
==============================================
# Licensed Materials - Property of IBM
# Copyright IBM Corp. 2018, 2019
==============================================
*/
/*
============================================================
This file contains commonly used C++ native functions.

First created on: Jun/10/2019
Last modified on: Jun/10/2019
============================================================
*/
#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

// Include this SPL file so that we can use the SPL functions and types in this C++ code.
#include <SPL/Runtime/Function/SPLFunctions.h>
#include <vector>
#include <sstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>

// Define a C++ namespace that will contain our native function code.
namespace cpp_util_functions {
	// By including this line, we will have access to the SPL namespace and anything defined within that.
	using namespace SPL;

	// Prototype for our native functions are declared here.
	int32 launch_app(rstring const & appName, rstring & resultStringOutput);

	// Inline native function to launch an external application within the SPL code.
	// This function takes two rstring arguments.
	// First argument is the one in which the caller must pass the
	// name of a Linux command or an application to launch (as a fully qualified path: /tmp/test/my_script.sh).
	inline int32 launch_app(rstring const & appName, rstring & resultStringOutput) {
	   FILE *fpipe;
	   int32 rc = 0;
	   int bufSize = 32*1024;
	   char outStreamResult[bufSize+100];
	   resultStringOutput = "";

	   // Open a pipe with the application name as provided by the caller.
	   fpipe = (FILE*)popen(appName.c_str(), "r");

	   if (!fpipe) {
		   SPLAPPTRC(L_INFO, "Failure while launching " + appName, "APP_LAUNCHER");
		   rc = 1;
		   return(rc);
	   } else {
		   SPLAPPTRC(L_DEBUG, "Successfully launched " + appName, "APP_LAUNCHER");
	   }

	   // If we opened the pipe in "r" mode, then we should wait here to
	   // fully read the stdout results coming from the launched application.
	   //
	   // NOTE: In case if we want to do the "w" mode, then it is necessary for us
	   // to feed the required input expected by the launched application via the
	   // stdin of the pipe.
	   while (fgets(outStreamResult, bufSize, fpipe) != NULL) {
		   SPLAPPTRC(L_TRACE, "Result from launched application: " +
			   std::string(outStreamResult), "APP_LAUNCHER");
		   resultStringOutput += std::string(outStreamResult);
	   }

	   // Close the pipe.
	   pclose (fpipe);
	   return(rc);
	}

}
#endif
