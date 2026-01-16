//=============================================================================
//
// Copyright(c) 2025 Lattice Semiconductor Corp. All rights reserved.
//
// These coded instructions, statements, and computer programs contain
// unpublished proprietary information written by LSCC and are protected 
// by copyright law. They may not be disclosed to third parties or copied 
// or duplicated in any form, in whole or in part, without the prior 
// written consent of LSCC.
//
//=============================================================================

#include "pch.h"

#include "EveControlInterface.h"
#include "EveCamera.h"
#include "EveFpga.h"
#include "EveImage.h"

#if defined(WIN32)
#include <combaseapi.h>
#include <mfapi.h>
#endif

using namespace std::chrono_literals;

static std::mutex callbackMutex;
static EveProcessingCallbackReturnData eveCallbackReturnData{};

static auto counter{ 0 };

// this is the data callback that will be called by EVE after a frame is processed
static void ProcessData( EveProcessingCallbackReturnData *returnData )
{
	std::lock_guard<std::mutex> lock{ callbackMutex };

	++counter;
	if( counter == 300 )
	{
		//after this functions returns, EVE will stop processing
		eveCallbackReturnData.requestedState = EVE_REQUESTED_PROCESSING_STATE_STOP;
	}

	auto fpgaData{ EveGetFpgaData() };
	if( fpgaData.data )
	{
		std::cout << "Number of users: " << fpgaData.data->pipelineData.dataContent.numberOfUsers << "\n";
	}

	*returnData = eveCallbackReturnData;
}

static int Run()
{
	//configuring some startup options of the SDK
	EveStartupParameters startParameters{};
	//we won't need to setup a GPU since algorithms are running on the FPGA
	startParameters.gpuPreference = EveGpuPreference::EVE_NO_GPU;
	//we let the EVE SDK connect to the camera and consume the video feed
	//and it allows the use of the callback defined above
	startParameters.imageProvider = EveImageProvider::EVE_CAMERA;

	//create EVE
	EveError errorCode = CreateEve( startParameters );
	if( errorCode != EVE_ERROR_NO_ERROR )
	{
		std::cout << "CreateEve error code: " << errorCode << "\n";
		return static_cast<int>( errorCode );
	}

	//get the number of cameras currently detected by the system
	auto numberOfCamers{ EveGetNumberOfCameras() };
	if( numberOfCamers.error != EVE_ERROR_NO_ERROR )
	{
		std::cout << "Number of cameras error code: " << errorCode << "\n";
		return static_cast<int>( errorCode );
	}

	//loop through all cameras and save the index of the FPGA camera
	auto count{ numberOfCamers.count };
	auto found{ false };
	auto cameraIndex{ 0u };
	for( auto n{ 0u }; n < count; ++n )
	{
		auto camera{ EveGetCamera( n ) };
		if( camera.data.isFpgaCamera == 1 )
		{
			cameraIndex = n;
			found = true;
			break;
		}
	}

	if( !found )
	{
		std::cout << "No FPGA camera detected, shutting down\n";
		return 1;
	}

	//recommended resolutions are 1600x1200 or 3200x2400
	CCameraFormat format{};
	format.resolution.width = 1600;
	format.resolution.height = 1200;
	EveSetCamera( cameraIndex, format );
	
	//configuring some additional options for the FPGA
	EveFpgaOptions options{};

	//these numbers may change depending on your SOM configuration
	options.parameters.connection = EveFpgaConnectionType::EVE_FPGA_I2C;
	options.parameters.i2cAdapterNumber = 0;
	options.parameters.i2cDeviceNumber = 48;
	options.parameters.i2cIRQPin = 26;

	//always use pipeline version 4
	//the need to manually specify the version number will be removed in a future release
	options.parameters.pipelineVersion = 4;

	//disables the camera auto start / stop feature
	//the need to manually specify this will be removed in a future release
	options.parameters.forceCameraOn = 1;

	EveConfigureFpga( options );

	//for visualizing the data overlaid on top of the image, set this option to 1
	//if this is not needed, you can omit this configuration step
	EveFpgaDebugOptions debugOptions{};
	debugOptions.enableDrawingOnImage = 1;
	EveConfigureFpgaDebug( debugOptions );

	//setup the output data callback
	errorCode = EveRegisterDataCallback( ProcessData );
	if( errorCode != EVE_ERROR_NO_ERROR )
	{
		std::cout << "EveRegisterDataCallback error code: " << errorCode << std::endl;
		return static_cast<int>( errorCode );
	}

	//start the SDK
	errorCode = StartEve();
	if( errorCode != EVE_ERROR_NO_ERROR )
	{
		std::cout << "StartEve error code: " << errorCode << "\n";
		return static_cast<int>( errorCode );
	}

	//keep running the SDK until 
	while( eveCallbackReturnData.requestedState == EVE_REQUESTED_PROCESSING_STATE_CONTINUE )
	{
		//wait for the callback to request a processing stop
		std::this_thread::sleep_for( 1s );
	}

	ShutdownEve();

	return 0;
}


int main( int, char ** )
{
	std::ios::sync_with_stdio( false );

#if defined(WIN32)
	::CoInitializeEx( nullptr, COINIT_MULTITHREADED );
	::MFStartup( MF_VERSION );
#endif

	int result = 0;

	try
	{
		result = Run();
	}
	catch( std::exception const &error )
	{
		std::cout << "Exception thrown: " << error.what() << std::endl;
		result = -1;
	}

#if defined(WIN32)
	::MFShutdown();
	::CoUninitialize();
#endif

	std::cout << std::endl;

	return result;
}
