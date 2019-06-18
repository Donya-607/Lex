#ifndef _INCLUDED_LOADER_H_
#define _INCLUDED_LOADER_H_

#include <vector>

class Loader
{

public:
	Loader( const char *fileName );
	~Loader();
	Loader( const Loader &  ) = delete;
	Loader( const Loader && ) = delete;
	Loader & operator = ( const Loader &  ) = delete;
	Loader & operator = ( const Loader && ) = delete;
public:

};

#endif // _INCLUDED_LOADER_H_