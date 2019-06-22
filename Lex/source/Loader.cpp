#include "Loader.h"

#include <fbxsdk.h>
#include <functional>
#include <crtdbg.h>
#include <Windows.h>
#include <memory>

#include "Useful.h"

namespace FBX = fbxsdk;

namespace Donya
{

	Loader::Loader() :
		fileName(),
		indices(),
		normals(),
		positions()
	{

	}
	Loader::~Loader()
	{
		std::vector<size_t>().swap( indices );
		std::vector<Donya::Vector3>().swap( positions );
		std::vector<Donya::Vector3>().swap( normals );
	}

	bool Loader::Load( const std::string &filePath, std::string *outputErrorString )
	{
		bool isSuccess = true;

		MakeFileName( filePath );

		FBX::FbxManager		*pManager		= FBX::FbxManager::Create();
		FBX::FbxIOSettings	*pIOSettings	= FBX::FbxIOSettings::Create( pManager, IOSROOT );
		pManager->SetIOSettings( pIOSettings );

		FBX::FbxScene		*pScene			= FBX::FbxScene::Create( pManager, "" );
		#pragma region Import
		{
			FBX::FbxImporter *pImporter		= FBX::FbxImporter::Create( pManager, "" );
			if ( !pImporter->Initialize( fileName.c_str(), -1, pManager->GetIOSettings() ) )
			{
				isSuccess = false;

				if ( outputErrorString != nullptr )
				{
					*outputErrorString =  "Failed : FbxImporter::Initialize().\n";
					*outputErrorString += "Error message is : ";
					*outputErrorString += pImporter->GetStatus().GetErrorString();
				}

				goto UNINITIALIZE;
			}

			if ( !pImporter->Import( pScene ) )
			{
				isSuccess = false;

				if ( outputErrorString != nullptr )
				{
					*outputErrorString =  "Failed : FbxImporter::Import().\n";
					*outputErrorString += "Error message is :";
					*outputErrorString += pImporter->GetStatus().GetErrorString();
				}

				goto UNINITIALIZE;
			}

			pImporter->Destroy();
		}
		#pragma endregion

		

		UNINITIALIZE:

		pManager->Destroy();

		return isSuccess;
	}

	std::string GetUTF8FullPath( const std::string &inputFilePath, size_t filePathLength = 512U )
	{
		// reference to http://blog.livedoor.jp/tek_nishi/archives/9446152.html

		std::unique_ptr<char[]> fullPath = std::make_unique<char[]>( filePathLength );
		auto writeLength = GetFullPathNameA( inputFilePath.c_str(), filePathLength, fullPath.get(), nullptr );

		char *convertedPath = nullptr;
		FBX::FbxAnsiToUTF8( fullPath.get(), convertedPath );

		std::string convertedStr( convertedPath );

		FBX::FbxFree( convertedPath );

		return convertedStr;
	}
	void Loader::MakeFileName( const std::string &filePath )
	{
		constexpr size_t FILE_PATH_LENGTH = 512U;

		fileName = GetUTF8FullPath( filePath, FILE_PATH_LENGTH );
	}
}