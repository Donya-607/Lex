#include "Motion.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use ZeroEqual().

#include "Loader.h"

#undef max
#undef min

namespace Donya
{
	bool MotionChunk::Create( const Donya::Loader &loader, MotionChunk *pOutput )
	{
		if ( !pOutput ) { return false; }
		// else

		const std::vector<Donya::Loader::Motion> *pLoadedMotions = loader.GetMotions();
		if ( !pLoadedMotions ) { return false; }
		// else

		auto AssignSkeletal = []( Skeletal *pSkeletal, const Loader::Skeletal &assignSkeletal )
		{
			const size_t SKELETAL_COUNT = assignSkeletal.skeletal.size();
			pSkeletal->boneCount = SKELETAL_COUNT;
			pSkeletal->skeletal.resize( SKELETAL_COUNT );
			for ( size_t i = 0; i < SKELETAL_COUNT; ++i )
			{
				pSkeletal->skeletal[i].name			= assignSkeletal.skeletal[i].name;
				pSkeletal->skeletal[i].transform	= assignSkeletal.skeletal[i].transform;

			}
		};

		const size_t MOTION_COUNT = pLoadedMotions->size();
		std::vector<Motion> motions{ MOTION_COUNT };
		for ( size_t i = 0; i < MOTION_COUNT; ++i )
		{
			auto &argument	= ( *pLoadedMotions )[i];
			auto &myself	= motions[i];

			myself.meshNo		= argument.meshNo;
			myself.samplingRate	= argument.samplingRate;
			myself.names		= argument.names;

			const size_t SKELETAL_COUNT = argument.motion.size();
			myself.motion.resize( SKELETAL_COUNT );
			for ( size_t j = 0; j < SKELETAL_COUNT; ++j )
			{
				AssignSkeletal( &myself.motion[j], argument.motion[j] );
			}
		}

		bool succeeded = pOutput->Init( motions );
		return succeeded;
	}

	bool MotionChunk::Init( const std::vector<Motion> &motions )
	{
		if ( wasCreated ) { return false; }
		// else

		motionsPerMesh = motions;

		wasCreated = true;
		return true;
	}

	size_t MotionChunk::GetMotionCount() const
	{
		return motionsPerMesh.size();
	}
	Motion MotionChunk::FetchMotion( unsigned int motionIndex ) const
	{
		const Motion NIL{};

		if ( motionsPerMesh.empty() ) { return NIL; }
		// else

		if ( GetMotionCount() <= motionIndex )
		{
			_ASSERT_EXPR( 0, L"Error : out of range at MotionChunk." );
			return NIL;
		}
		// else

		return motionsPerMesh[motionIndex];
	}

	Animator::Animator() :
		elapsedTime(), samplingRate()
	{}
	Animator::~Animator() = default;

	void Animator::Init()
	{
		elapsedTime  = 0.0f;
		samplingRate = 0.0f;
	}
	void Animator::Update( float argElapsedTime )
	{
		elapsedTime += argElapsedTime;
	}

	void Animator::SetFrame( int frame, float extraSamplingRate )
	{
		float rate = ( ZeroEqual( extraSamplingRate ) ) ? samplingRate : extraSamplingRate;

		elapsedTime = scast<float>( frame ) * rate;
	}
	void Animator::SetSamplingRate( float rate )
	{
		samplingRate = rate;
	}

	// TODO:Change these "motion frame" type to float, then interpolate a fetched skeletal.

	int CalcFrameImpl( float elapsedTime, float rate )
	{
		return ( ZeroEqual( rate ) ) ? 0 : scast<int>( elapsedTime / rate );
	}
	int Animator::CalcCurrentFrame() const
	{
		return CalcFrameImpl( elapsedTime, samplingRate );
	}
	int Animator::CalcCurrentFrame( const Motion &motion, bool useWrapAround ) const
	{
		const float rate = ( ZeroEqual( samplingRate ) ) ? motion.samplingRate : samplingRate;

		int currentFrame = CalcFrameImpl( elapsedTime, rate );

		const int MOTION_COUNT = scast<int>( motion.motion.size() );
		if ( MOTION_COUNT <= currentFrame )
		{
			currentFrame = ( useWrapAround )
			? currentFrame % MOTION_COUNT
			: 0;
		}
		
		return currentFrame;
	}

	Skeletal Animator::FetchCurrentMotion( const Motion &motion, bool useWrapAround ) const
	{
		if ( motion.motion.empty() )
		{
			// Return identities.
			return Skeletal{};
		}
		// else

		int currentFrame = CalcCurrentFrame( motion, useWrapAround );
		    currentFrame = std::max( 0, currentFrame ); // Fail safe

		return motion.motion[currentFrame];
	}
}
