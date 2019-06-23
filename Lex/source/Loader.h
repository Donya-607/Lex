#ifndef _INCLUDED_LOADER_H_
#define _INCLUDED_LOADER_H_

#include <string>
#include <vector>

#include "Vector.h"

namespace Donya
{

	class Loader
	{
	private:
		std::string fileName;
		std::vector<size_t>			indices;
		std::vector<Donya::Vector3>	normals;
		std::vector<Donya::Vector3>	positions;
	public:
		Loader();
		~Loader();
		// Loader( const Loader &  ) = delete;
		// Loader( const Loader && ) = delete;
		// Loader & operator = ( const Loader &  ) = delete;
		// Loader & operator = ( const Loader && ) = delete;
	public:
		/// <summary>
		/// outputErrorString can set nullptr.
		/// </summary>
		bool Load( const std::string &filePath, std::string *outputErrorString );
	public:
		std::string GetFileName() const { return fileName; }
	private:
		void MakeFileName( const std::string &filePath );
	};

}

#endif // _INCLUDED_LOADER_H_