# Developing STT Gateway Toolkit

## Get a working copy of the STT Gateway Toolkit

Goto your git working place e.g.:

	cd $HOME/git

Clone the repository from github

	git clone https://github.com/IBMStreams/streamsx.sttgateway.git

## Setup a Eclipse Project

Activate the Git-View and locate the repository of the STT Gateway toolkit. From the *Git Repositories* window 
click: Import projects -> import general project

## Setup a Eclipse CDT Project

Import the project in directory:

	<Your Workspace>/streamsx.sttgateway/com.ibm.streamsx.sttgateway

To enable the c++11 setting for the indexer open the properties of the project and:

* Goto : `C/C++ General -> Preprocessor Include Paths, Macros etc.`
* Here click on `Providers` and select `CDT GCC Built-in Compiler Settings` (This option must be enabled)
* Add the option `-std=c++11` to the *Command to get compiler specs:*
* Apply the changes.

Now you must provide all dependencies for the toolkit code. Execute

	ant requirements

Check whether all required library header files exist in directory `com.ibm.streamsx.sttgatewayit/include`

Refresh the project view (press F5).

Now you must re-build the index with right-click the project -> index -> Rebuild

