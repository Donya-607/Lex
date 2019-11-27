#pragma once

#include <assert.h>
#include <fstream>
#include <memory>
#include <sstream>

#undef max
#undef min

#include "cereal/cereal.hpp"
#include "cereal/archives/binary.hpp"
#include "cereal/archives/json.hpp"

namespace Donya
{
	class Serializer
	{
	public:
		enum Extension
		{
			BINARY = 0,
			JSON,
		};
	private:	// Use for Begin() ~ End() process.
		Extension	ext;
		std::unique_ptr<std::ifstream>					pIfs;
		std::unique_ptr<std::ofstream>					pOfs;
		std::unique_ptr<std::stringstream>				pSS;
		std::unique_ptr<cereal::BinaryInputArchive>		pBinInArc;
		std::unique_ptr<cereal::JSONInputArchive>		pJsonInArc;
		std::unique_ptr<cereal::BinaryOutputArchive>	pBinOutArc;
		std::unique_ptr<cereal::JSONOutputArchive>		pJsonOutArc;
		bool isValid;	// It will be true while Begin() ~ End(), else false.
	public:
		Serializer() : ext( BINARY ), pIfs( nullptr ), pSS( nullptr ), pBinInArc( nullptr ), pJsonInArc( nullptr ), pBinOutArc( nullptr ), pJsonOutArc( nullptr ), isValid( false )
		{}
	public:
		template<class SerializeObject>
		bool Load( Extension extension, const char *filePath, const char *objectName, SerializeObject &instance ) const
		{
			auto openMode = ( extension == Extension::BINARY ) ? std::ios::in | std::ios::binary : std::ios::in;
			std::ifstream ifs( filePath, openMode );
			if ( !ifs.is_open() ) { return false; }
			// else

			std::stringstream ss{};
			ss << ifs.rdbuf();

			switch ( extension )
			{
			case BINARY:
				{
					cereal::BinaryInputArchive binInArchive( ss );
					binInArchive( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case JSON:
				{
					cereal::JSONInputArchive jsonInArchive( ss );
					jsonInArchive( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			ifs.close();
			ss.clear();

			return true;
		}

		template<class SerializeObject>
		bool Save( Extension extension, const char *filePath, const char *objectName, SerializeObject &instance ) const
		{
			std::stringstream ss{};

			switch ( extension )
			{
			case BINARY:
				{
					cereal::BinaryOutputArchive binOutArchive( ss );
					binOutArchive( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case JSON:
				{
					cereal::JSONOutputArchive jsonOutArchive( ss );
					jsonOutArchive( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			auto openMode = ( extension == Extension::BINARY ) ? std::ios::out | std::ios::binary : std::ios::out;
			std::ofstream ofs( filePath, openMode );
			if ( !ofs.is_open() ) { return false; }
			// else

			ofs << ss.str();

			ofs.close();
			ss.clear();

			return true;
		}
	public:
		bool LoadBegin( Extension extension, const char *filePath )
		{
			if ( isValid )
			{
				/*
				If you called LoadBegin(),
				you must call the LoadEnd() after the LoadBegin().
				*/
				assert( 0 && "Serializer : Error ! LoadBegin() called before LoadEnd() !" );
				return false;
			}
			// else

			auto openMode = ( extension == Extension::BINARY ) ? std::ios::in | std::ios::binary : std::ios::in;
			pIfs = std::make_unique<std::ifstream>( filePath, openMode );
			if ( !pIfs->is_open() ) { return false; }
			// else

			pSS = std::make_unique<std::stringstream>();
			*pSS << pIfs->rdbuf();

			ext = extension;
			switch ( extension )
			{
			case BINARY:
				pBinInArc  = std::make_unique<cereal::BinaryInputArchive>( *pSS );
				break;
			case JSON:
				pJsonInArc = std::make_unique<cereal::JSONInputArchive>( *pSS );
				break;
			default:
				return false;
			}

			isValid = true;

			return true;
		}

		template<class SerializeObject>
		bool LoadPart( const char *objectName, SerializeObject &instance )
		{
			if ( !isValid ) { return false; }
			// else

			switch ( ext )
			{
			case BINARY:
				{
					( *pBinInArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case JSON:
				{
					( *pJsonInArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			return true;
		}

		void LoadEnd()
		{
			if ( !isValid ) { return; }
			// else

			switch ( ext )
			{
			case BINARY:
				pBinInArc.reset( nullptr );
				break;
			case JSON:
				pJsonInArc.reset( nullptr );
				break;
			default:
				break;
			}

			pIfs->close();
			pSS->clear();

			pIfs.reset( nullptr );
			pSS.reset( nullptr );

			isValid  = false;
		}
	public:
		bool SaveBegin( Extension extension, const char *filePath )
		{
			if ( isValid )
			{
				/*
				If you called SaveBegin(),
				you must call the SaveEnd() after the SaveBegin().
				*/
				assert( 0 && "Serializer : Error ! SaveBegin() called before SaveEnd() !" );
				return false;
			}
			// else

			pSS = std::make_unique<std::stringstream>();

			ext = extension;
			switch ( extension )
			{
			case BINARY:
				pBinOutArc  = std::make_unique<cereal::BinaryOutputArchive>( *pSS );
				break;
			case JSON:
				pJsonOutArc = std::make_unique<cereal::JSONOutputArchive>( *pSS );
				break;
			default:
				return false;
			}

			auto openMode = ( extension == Extension::BINARY ) ? std::ios::out | std::ios::binary : std::ios::out;
			pOfs = std::make_unique<std::ofstream>( filePath, openMode );
			if ( !pOfs->is_open() ) { return false; }
			// else

			isValid = true;

			return true;
		}

		template<class SerializeObject>
		bool SavePart( const char *objectName, SerializeObject &instance )
		{
			if ( !isValid ) { return false; }
			// else

			switch ( ext )
			{
			case BINARY:
				{
					( *pBinOutArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			case JSON:
				{
					( *pJsonOutArc )( cereal::make_nvp( objectName, instance ) );
				}
				break;
			default:
				return false;
			}

			return true;
		}

		void SaveEnd()
		{
			if ( !isValid ) { return; }
			// else

			switch ( ext )
			{
			case BINARY:
				pBinOutArc.reset( nullptr );
				break;
			case JSON:
				pJsonOutArc.reset( nullptr );
				break;
			default:
				break;
			}

			*pOfs << pSS->str();

			pOfs->close();
			pSS->clear();

			pOfs.reset( nullptr );
			pSS.reset( nullptr );

			isValid = false;
		}
	public:
		// Static helper methods.

		/// <summary>
		/// "instance" : The object's instance that you want serialize. This object's status must be the same as when saving.<para></para>
		/// "filePath" : The load file path that also contain extension.<para></para>
		/// "objectName" : This name use as identifier. This name must be the same as when saving.<para></para>
		/// "fromBinary" : [TRUE:Load from binary file] [FALSE: Load from json file]
		/// </summary>
		template<class SerializeObject>
		static bool Load( SerializeObject &instance, const char *filePath, const char *objectName, bool fromBinary )
		{
			Extension ext = ( fromBinary )
			? Serializer::Extension::BINARY
			: Serializer::Extension::JSON;
			
			Serializer seria{};
			return seria.Load( ext, filePath, objectName, instance );
		}
		/// <summary>
		/// "instance" : The object's instance that you want serialize.<para></para>
		/// "filePath" : The save file path that also contain extension.<para></para>
		/// "objectName" : This name use as identifier. Please use same identifier at load.<para></para>
		/// "toBinary" : Do you save to binary file?
		/// </summary>
		template<class SerializeObject>
		static bool Save( SerializeObject &instance, const char *filePath, const char *objectName, bool toBinary )
		{
			Serializer::Extension ext = ( toBinary )
			? Serializer::Extension::BINARY
			: Serializer::Extension::JSON;
			
			Serializer seria{};
			return seria.Save( ext, filePath, objectName, instance );
		}
	};
}
