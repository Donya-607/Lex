#include "Motion.h"

#include "Donya/Constant.h"		// Use scast macro.
#include "Donya/Quaternion.h"	// Use for interpolate.
#include "Donya/Useful.h"		// Use ZeroEqual().

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
		elapsedTime(), samplingRate(),
		enableInterpolate( false )
	{}

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

	void Animator::SetInterpolateFlag( bool useInterpolate )
	{
		enableInterpolate = useInterpolate;
	}

	void Animator::SetCurrentElapsedTime( float overwrite )
	{
		elapsedTime = overwrite;
	}
	float Animator::GetCurrentElapsedTime()
	{
		return elapsedTime;
	}

	float CalcFrameImpl( float elapsedTime, float rate )
	{
		return ( ZeroEqual( rate ) ) ? 0.0f : ( elapsedTime / rate );
	}
	float Animator::CalcCurrentFrame() const
	{
		return CalcFrameImpl( elapsedTime, samplingRate );
	}
	float Animator::CalcCurrentFrame( const Motion &motion, bool useWrapAround ) const
	{
		const float motionCountF = scast<float>( motion.motion.size() );
		if ( ZeroEqual( motionCountF ) ) { return 0.0f; }
		// else

		const float rate   = ( ZeroEqual( samplingRate ) ) ? motion.samplingRate : samplingRate;

		float currentFrame =  CalcFrameImpl( elapsedTime, rate );
		if (  motionCountF <= currentFrame )
		{
			currentFrame = ( useWrapAround )
			? fmodf( currentFrame, motionCountF )
			: 0.0f;
		}

		return currentFrame;
	}

	Skeletal Animator::FetchCurrentPose( const Motion &motion, bool useWrapAround ) const
	{
		if ( motion.motion.empty() )
		{
			// Return identities.
			return Skeletal{};
		}
		// else
		const int motionCount = scast<int>( motion.motion.size() );
		if ( motionCount == 1 )
		{
			return motion.motion[0];
		}
		// else

		float currentFrame = CalcCurrentFrame( motion, useWrapAround );
		      currentFrame = std::max( 0.0f, currentFrame ); // Fail safe

		if ( enableInterpolate )
		{
			float		integral{};
			float		fractional		= modf( currentFrame, &integral );
			int			baseFrame		= scast<int>( integral );
			int			nextFrame		= ( motionCount <= baseFrame + 1 )
										? ( baseFrame + 1 ) % motionCount // Wrap around.
										: baseFrame + 1;

			Skeletal	currentPose		= motion.motion[baseFrame];
			Skeletal	nextPose		= motion.motion[nextFrame];

			_ASSERT_EXPR( currentPose.boneCount == nextPose.boneCount, L"Error : The bone count did not match! " );

			/* Use for slerp.
			Donya::Vector3		scaling{};
			Donya::Vector3		translation{};
			Donya::Vector4x4	rotation{};
			Donya::Quaternion	rotationSlerped{};
			enum { Base = 0, Next };
			Donya::Vector3		extracted[2]{};
			Donya::Vector4x4	rotationMat[2]{};
			Donya::Quaternion	rotationQuat[2]{};
			enum { X = 0, Y, Z };
			Donya::Vector3		baseScales[3]{};
			Donya::Vector3		nextScales[3]{};
			*/

			Skeletal	interpolated	= currentPose;
			for ( size_t i = 0; i < interpolated.boneCount; ++i )
			{
				const	auto &next	= nextPose.skeletal[i].transform;
						auto &base	= interpolated.skeletal[i].transform;

				// Currently using a lerp.
				// TODO : Implement Slerp a matrix.
				{
					base = Lerp( base, next, fractional );
					continue;
				}
				// else

				/* Use for slerp.
				extracted[Base]		= Donya::Vector3{ base._41, base._42, base._43 };
				extracted[Next]		= Donya::Vector3{ next._41, next._42, next._43 };
				translation			= Lerp( extracted[Base], extracted[Next], fractional );
				
				baseScales[X]		= Donya::Vector3{ base._11, base._12, base._13 };
				baseScales[Y]		= Donya::Vector3{ base._21, base._22, base._23 };
				baseScales[Z]		= Donya::Vector3{ base._31, base._32, base._33 };
				nextScales[X]		= Donya::Vector3{ next._11, next._12, next._13 };
				nextScales[Y]		= Donya::Vector3{ next._21, next._22, next._23 };
				nextScales[Z]		= Donya::Vector3{ next._31, next._32, next._33 };
				extracted[Base].x	= baseScales[X].Length();
				extracted[Base].y	= baseScales[Y].Length();
				extracted[Base].z	= baseScales[Z].Length();
				extracted[Next].x	= nextScales[X].Length();
				extracted[Next].y	= nextScales[Y].Length();
				extracted[Next].z	= nextScales[Z].Length();
				scaling				= Lerp( extracted[Base], extracted[Next], fractional );
				
				baseScales[X].Normalize();
				baseScales[Y].Normalize();
				baseScales[Z].Normalize();
				nextScales[X].Normalize();
				nextScales[Y].Normalize();
				nextScales[Z].Normalize();
				rotationMat[Base]	= Donya::Vector4x4::MakeRotationOrthogonalAxis
				(
					baseScales[X],
					baseScales[Y],
					baseScales[Z]
				);
				rotationMat[Next]	= Donya::Vector4x4::MakeRotationOrthogonalAxis
				(
					nextScales[X],
					nextScales[Y],
					nextScales[Z]
				);
				rotationQuat[Base]	= Donya::Quaternion::Make ( rotationMat[Base] ).Normalized();
				rotationQuat[Next]	= Donya::Quaternion::Make ( rotationMat[Next] ).Normalized();
				rotationSlerped		= Donya::Quaternion::Slerp( rotationQuat[Base], rotationQuat[Next], fractional ).Normalized();
				rotation			= rotationSlerped.RequireRotationMatrix();

				base = Donya::Vector4x4
				{
					rotation._11 * scaling.x,	rotation._12,				rotation._13,				0.0f,
					rotation._21,				rotation._22 * scaling.y,	rotation._33,				0.0f,
					rotation._31,				rotation._32,				rotation._33 * scaling.z,	0.0f,
					translation.x,				translation.y,				translation.z,				1.0f
				};
				*/
			}
			
			return		interpolated;
		}
		// else

		// Rounded down the decimal.
		return motion.motion[scast<int>( currentFrame )];
	}
}
