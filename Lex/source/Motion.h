#pragma once

#include <string>
#include <vector>

#include "Donya/Vector.h"

// Program version : 0

namespace Donya
{
	class Loader;

	/// <summary>
	/// Rig. Controller.
	/// </summary>
	struct Bone
	{
		std::string			name{};
		Donya::Vector4x4	transform{};
	};
	/// <summary>
	/// Gathering of bones(I call "skeletal"). This represents a posture at that time.
	/// </summary>
	struct Skeletal
	{
		size_t				boneCount{};
		std::vector<Bone>	skeletal{};
	};
	/// <summary>
	/// Gathering of skeletals(I call "Motion"). This represents a motion(animation).
	/// </summary>
	struct Motion
	{
	public:
		static constexpr float DEFAULT_SAMPLING_RATE = 1.0f / 24.0f;
	public:
		int							meshNo{};	// 0-based.
		float						samplingRate{ DEFAULT_SAMPLING_RATE };
		std::vector<std::string>	names{};
		std::vector<Skeletal>		motion{};	// Store consecutive skeletals according to a time.
	};

	/// <summary>
	/// This class contain a motions per mesh.
	/// </summary>
	class MotionChunk
	{
	public:
		/// <summary>
		/// Create from Loader object.<para></para>
		/// if create failed, or already loaded, returns false.
		/// </summary>
		static bool Create( const Donya::Loader &loader, MotionChunk *pOutput );
	private:
		std::vector<Motion> motionsPerMesh{};
		bool wasCreated{ false };
	private:
		bool Init( const std::vector<Motion> &motions );
	public:
		size_t GetMotionCount() const;
		/// <summary>
		/// The "motionNumber" link to mesh number.
		/// </summary>
		Motion FetchMotion( unsigned int motionNumber ) const;
	};

	/// <summary>
	/// This class is used in conjunction with "Motion" class.<para></para>
	/// This "Animator" can calculate a current animation frame and skeletal.
	/// </summary>
	class Animator
	{
	private:
		float	elapsedTime;
		float	samplingRate;		// Default is zero.
		bool	enableInterpolate;	// Default is false.
	public:
		Animator();
	public:
		/// <summary>
		/// Set zero to current frame(elapsedTime) and "samplingRate".
		/// </summary>
		void Init();

		/// <summary>
		/// Update a motion frame.
		/// </summary>
		void Update( float elapsedTime );
	public:
		/// <summary>
		/// Set current frame with registered sampling rate. If you want use another sampling rate, set value of not zero to "extraSamplingRate"(zero is specify to "use registered rate").
		/// </summary>
		void SetFrame( int frame, float extraSamplingRate = 0.0f );
		/// <summary>
		/// If set zero, I use specified motion's sampling rate at "FetchCurrentPose()".
		/// </summary>
		void SetSamplingRate( float rate );

		/// <summary>
		/// [TRUE:useInterpolate] The method of calculating a pose will also calculate an interpolation of a pose.<para></para>
		/// [FALSE:useInterpolate] The method of calculating a pose will calculate only a pose of the current frame(the less-equal than decimal is truncated).
		/// </summary>
		void SetInterpolateFlag( bool useInterpolate );

		/// <summary>
		/// Overwrite an internal timer that updating at Update(). This does not represent a current frame.
		/// </summary>
		void SetCurrentElapsedTime( float overwrite );
		/// <summary>
		/// Returns an internal timer that updating at Update(). This does not represent a current frame.
		/// </summary>
		float GetCurrentElapsedTime();
	public:
		/// <summary>
		/// Returns current motion frame calculated by registered sampling rate.
		/// </summary>
		float CalcCurrentFrame() const;
		/// <summary>
		/// Returns current motion frame calculated by registered sampling rate(if the zero is registered, use the motion's sampling rate).<para></para>
		/// [TRUE:useWrapAround] Returns frame will be wrap-arounded in motion count(ex.if motion count is 3, returns frame number is only 0, 1 or 2).<para></para>
		/// [FALSE:useWrapAround] Returns frame is zero if over than motion count.
		/// </summary>
		float CalcCurrentFrame( const Motion &motion, bool useWrapAround = true ) const;

		Skeletal FetchCurrentPose( const Motion &motion, bool useWrapAround = true ) const;
	};
}
