#pragma once

#include <string>
#include <vector>

#include "Donya/Vector.h"

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
	class Motion
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
		float elapsedTime;
		float samplingRate;
	public:
		Animator();
		~Animator();
	public:
		/// <summary>
		/// Set zero to current frame(elapsedTime) and "samplingRate".
		/// </summary>
		void Init();
		void Update( float elapsedTime );
	public:
		/// <summary>
		/// Set current frame with registered sampling rate. If you want use another sampling rate, set value of not zero to "extraSamplingRate"(zero is specify to "use registered rate").
		/// </summary>
		void SetFrame( int frame, float extraSamplingRate = 0.0f );
		/// <summary>
		/// If set zero, I use specified motion's sampling rate at "FetchCurrentMotion()".
		/// </summary>
		void SetSamplingRate( float rate );
	public:
		/// <summary>
		/// Returns current motion frame calculated by registered sampling rate.
		/// </summary>
		int CalcCurrentFrame() const;
		/// <summary>
		/// Returns current motion frame calculated by registered sampling rate.<para></para>
		/// [TRUE:useWrapAround] Returns frame will be wrap-arounded in motion count(ex.if motion count is 3, returns frame number is only 0, 1 or 2).<para></para>
		/// [FALSE:useWrapAround] Returns frame is zero if over than motion count.
		/// </summary>
		int CalcCurrentFrame( const Motion &motion, bool useWrapAround = true ) const;

		Skeletal FetchCurrentMotion( const Motion &motion, bool useWrapAround = true ) const;
	};
}
