# README --  FrameworkTests

This directory provides an automatic test for the sttgateway toolkit.

## Test Execution

To start the full test execute:
`./runTest.sh`

To start a quick test, execute:
`./runTest.sh --category quick`

This script installs the test framework in directory `scripts` and starts the test execution. The script delivers the following result codes:  
0     : all tests Success
20    : at least one test fails
25    : at least one test error
26    : Error during suite execution
130   : SIGINT received
other : another fatal error has occurred

More options are available and explained with command:
`./runTest.sh --help`

## Test Sequence

The `runTest.sh` installs the test framework into directory `scripts` and starts the test framework. The test framework 
checks if there is a running Streams instance. 

If the Streams instance is not running, a domain and an instance is created from the scratch and started. You can force the 
creation of instance and domain with command line option `--clean`

## Requirements

The test framework requires an valid Streams installation and environment and a running mail-server.

The `Runtests` test suite requires the service url and the service credentials of an stt service on IBM Cloud.
The location of the stt toolkit to test and the stt-service credentilal are controlled with properties. 
The location of standard properties file is 
`tests/TestProperties.sh`.

Use command line option `-D <name>=<value>` to set external variables or provide a new properties file with command line option 
`--properties <filename>`.

For further information look into the provided file `tests/TestProperties.sh`
