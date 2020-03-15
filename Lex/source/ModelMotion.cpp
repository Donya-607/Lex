#include "ModelMotion.h"

#include "Donya/Constant.h"	// Use scast macro.
#include "Donya/Useful.h"	// Use ZeroEqual().

namespace
{
	static constexpr float FPSToRate( float FPS )
	{
		return 1.0f / FPS;
	}
	static float ClampFrame		( float frame, float minFrame, float maxFrame )
	{
		return std::max( minFrame, std::min( maxFrame, frame ) );
	}
	static float WrapAroundFrame( float frame, float minFrame, float maxFrame )
	{
		const float diff = maxFrame - minFrame;
		if ( ZeroEqual( diff ) ) { return 0.0f; }
		// else
		return minFrame + fmodf( frame, diff );
	}
	static float HandleFrameImpl( float frame, float minFrame, float maxFrame, bool enableWrapAround )
	{
		return	( enableWrapAround )
				? WrapAroundFrame	( frame, minFrame, maxFrame )
				: ClampFrame		( frame, minFrame, maxFrame );
	}
}


namespace Donya
{
	namespace Model
	{
		Animation::KeyFrame	EmptyKeyFrame()
		{
			return Animation::KeyFrame{};
		}
		Animation::Motion	EmptyMotion()
		{
			return Animation::Motion{};
		}

		size_t MotionHolder::GetMotionCount() const
		{
			return motions.size();
		}
		bool MotionHolder::IsOutOfRange( int motionIndex ) const
		{
			if ( motionIndex < 0 ) { return true; }
			if ( scast<int>( GetMotionCount() ) <= motionIndex ) { return true; }
			// else

			return false;
		}

		Animation::Motion MotionHolder::GetMotion( int motionIndex ) const
		{
			if ( IsOutOfRange( motionIndex ) ) { return EmptyMotion(); }
			// else
			return motions[motionIndex];
		}
		Animation::Motion MotionHolder::FindMotion( const std::string &motionName ) const
		{
			for ( const auto &it : motions )
			{
				if ( it.name == motionName )
				{
					return it;
				}
			}

			return EmptyMotion();
		}

		void MotionHolder::AppendMotion( const Animation::Motion &element )
		{
			motions.emplace_back( element );
		}


		void  Animator::ResetTimer()
		{
			elapsedTime = 0.0f;
		}
		void  Animator::Update( float argElapsedTime )
		{
			elapsedTime += argElapsedTime;
		}

		void  Animator::AssignFrame( float frame )
		{
			elapsedTime = frame * FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame() const
		{
			return elapsedTime / FPSToRate( FPS );
		}
		float Animator::CalcCurrentFrame( float minFrame, float maxFrame ) const
		{
			const float frame = CalcCurrentFrame();

			return HandleFrameImpl( frame, minFrame, maxFrame, enableWrapAround );
		}
		float Animator::CalcCurrentFrame( const std::vector<Animation::KeyFrame> &asFrameRange ) const
		{
			const size_t frameCount = asFrameRange.size();
			if ( !frameCount || frameCount == 1 )
			{
				return 0.0f;
			}
			// else
			return CalcCurrentFrame( 0.0f, scast<float>( frameCount ) );
		}

		Animation::KeyFrame Animator::CalcCurrentPose( const std::vector<Animation::KeyFrame> &motion ) const
		{
			if ( motion.empty()     ) { return Animation::KeyFrame{}; } // Returns empty.
			if ( motion.size() == 1 ) { return motion.front(); }
			// else

			auto CalcWholeSeconds = []( const std::vector<Animation::KeyFrame> &motion )
			{
				// The "seconds" contain the begin seconds(not playing seconds).
				return motion.back().seconds;
				/*
				float sum = 0.0f;
				for ( const auto &it : motion )
				{
					sum += it.seconds;
				}
				return sum;
				*/
			};
			const float wholeSeconds = CalcWholeSeconds( motion );

			float currentSeconds = elapsedTime;
			if ( wholeSeconds <= currentSeconds )
			{
				if ( !enableWrapAround ) { return motion.back(); }
				// else

				currentSeconds = fmodf( currentSeconds, wholeSeconds );
			}

			Animation::KeyFrame rv;

			const size_t motionCount = motion.size();
			for ( size_t i = 0; i < motionCount - 1; ++i )
			{
				const auto &keyFrameL = motion[i];
				const auto &keyFrameR = motion[i + 1];
				if ( currentSeconds < keyFrameL.seconds || keyFrameR.seconds <= currentSeconds ) { continue; }
				// else

				const float diffL = currentSeconds    - keyFrameL.seconds;
				const float diffR = keyFrameR.seconds - keyFrameL.seconds;
				// const float percent = diffL / ( diffR + EPSILON/* Prevent zero-divide */ );
				const float percent = ( currentSeconds - keyFrameL.seconds / keyFrameR.seconds - keyFrameL.seconds );

				rv = Animation::KeyFrame::Interpolate( keyFrameL, keyFrameR, percent );

				break;
			}

			return rv;
		}
		Animation::KeyFrame Animator::CalcCurrentPose( const Animation::Motion &motion ) const
		{
			return CalcCurrentPose( motion.keyFrames );
		}

		void  Animator::EnableWrapAround()
		{
			enableWrapAround = true;
		}
		void  Animator::DisableWrapAround()
		{
			enableWrapAround = false;
		}

		void  Animator::SetFPS( float overwrite )
		{
			FPS = overwrite;
			FPS = std::max( 1.0f, FPS );
		}

		void  Animator::SetInternalElapsedTime( float overwrite )
		{
			elapsedTime = overwrite;
		}
		float Animator::GetInternalElapsedTime() const
		{
			return elapsedTime;
		}


		//bool FocusMotion::IsValidMotion( const Animation::Motion &motion )
		//{
		//	/*
		//	Requirements:
		//		1. The motion has something.
		//		2. The motion's bone count of all key pose of keyframes is the same.
		//	*/

		//	// No.1
		//	if ( motion.keyFrames.empty() ) { return false; };
		//	// else

		//	const auto  &someFrame = motion.keyFrames.front();
		//	const size_t boneCount = someFrame.keyPose.size();

		//	// No.2
		//	for ( const auto &keyFrame : motion.keyFrames )
		//	{
		//		if ( keyFrame.keyPose.size() != boneCount )
		//		{
		//			return false;
		//		}
		//	}
		//	
		//	return true;
		//}

		//bool FocusMotion::RegisterMotion( const Animation::Motion &target )
		//{
		//	if ( !IsValidMotion( target ) )
		//	{
		//		focus = EmptyMotion();

		//		skeletal.clear();
		//		return false;
		//	}
		//	// else

		//	focus = target;

		//	AdaptToFocus();
		//	return true;
		//}

		//Animation::Motion FocusMotion::GetFocusingMotion() const
		//{
		//	return focus;
		//}
		//const std::vector<FocusMotion::Node> &FocusMotion::GetCurrentSkeletal() const
		//{
		//	return skeletal;
		//}

		//bool FocusMotion::UpdateCurrentSkeletal( float currentFrame )
		//{
		//	if ( !IsValidMotion( focus ) ) { return false; }
		//	// else

		//	const auto currentPose = CalcCurrentPose( currentFrame );
		//	UpdateSkeletal( currentPose );

		//	CalcLocalMatrix();

		//	CalcGlobalMatrix();

		//	return true;
		//}
		//bool FocusMotion::UpdateCurrentSkeletal( const Animator &animator )
		//{
		//	const float currentFrame = animator.CalcCurrentFrame();
		//	return UpdateCurrentSkeletal( currentFrame );
		//}

		//void FocusMotion::AdaptToFocus()
		//{
		//	const auto   basicKeyFrame	= CalcCurrentPose( 0.0f );
		//	const size_t boneCount		= basicKeyFrame.keyPose.size();

		//	if ( !boneCount )
		//	{
		//		skeletal.clear();
		//		return;
		//	}
		//	// else

		//	skeletal.resize( boneCount );
		//	UpdateSkeletal( basicKeyFrame );
		//}

		//Animation::KeyFrame FocusMotion::CalcCurrentPose( float currentFrame )
		//{
		//	const size_t frameCount = focus.keyFrames.size();

		//	float  integral{};
		//	float  fractional = modf( currentFrame, &integral );	// Will using as percent of interpolation.
		//	size_t baseFrame  = scast<size_t>( integral ) % frameCount;
		//	size_t nextFrame  = ( frameCount <= baseFrame + 1 )
		//						? ( baseFrame + 1 ) % frameCount	// Wrap around.
		//						: baseFrame + 1;

		//	auto SlerpTransform	= []( const Animation::Transform &lhs, const Animation::Transform &rhs, float percent )
		//	{
		//		Animation::Transform rv = lhs;
		//		rv.scale		= Donya::Lerp( lhs.scale, rhs.scale, percent );
		//		rv.rotation		= Donya::Quaternion::Slerp( lhs.rotation, rhs.rotation, percent );
		//		rv.translation	= Donya::Lerp( lhs.translation, rhs.translation, percent );
		//		return rv;
		//	};

		//	const Animation::KeyFrame currentPose	= focus.keyFrames[baseFrame];
		//	const Animation::KeyFrame nextPose		= focus.keyFrames[nextFrame];

		//	// Interpolation is not necessary.
		//	if ( ZeroEqual( fractional ) ) { return currentPose; }
		//	// else

		//	auto SlerpBone = [&SlerpTransform]( const Animation::Bone &lhs, const Animation::Bone rhs, float percent )
		//	{
		//		Animation::Bone rv		= lhs;
		//		rv.transform			= SlerpTransform( lhs.transform,			rhs.transform,			percent );
		//		rv.transformToParent	= SlerpTransform( lhs.transformToParent,	rhs.transformToParent,	percent );
		//		return rv;
		//	};

		//	// A member that does not interpolate is using as currentPose.
		//	Animation::KeyFrame resultPose = currentPose;

		//	_ASSERT_EXPR( currentPose.keyPose.size() == nextPose.keyPose.size(), L"Error : The bone count did not match! " );

		//	const size_t boneCount = currentPose.keyPose.size();
		//	for ( size_t i = 0; i < boneCount; ++i )
		//	{
		//		resultPose.keyPose[i] = SlerpBone( currentPose.keyPose[i], nextPose.keyPose[i], fractional );
		//	}

		//	return resultPose;
		//}

		//void FocusMotion::UpdateSkeletal( const Animation::KeyFrame &keyFrame )
		//{
		//	_ASSERT_EXPR( keyFrame.keyPose.size() == skeletal.size(), L"Error : The bone count of focusing motion is invalid!" );

		//	const size_t boneCount = skeletal.size();
		//	for ( size_t i = 0; i < boneCount; ++i )
		//	{
		//		skeletal[i].bone = keyFrame.keyPose[i];
		//	}
		//}

		//void FocusMotion::CalcLocalMatrix()
		//{
		//	for ( auto &it : skeletal )
		//	{
		//		it.local = it.bone.transform.ToWorldMatrix();
		//	}
		//}
		//void FocusMotion::CalcGlobalMatrix()
		//{
		//	for ( auto &it : skeletal )
		//	{
		//		if ( it.bone.parentIndex == -1 )
		//		{
		//			it.global = it.local;
		//		}
		//		else
		//		{
		//			const auto &parentBone = skeletal[it.bone.parentIndex];
		//			it.global = it.local * parentBone.global;
		//		}
		//	}
		//}
	}
}
