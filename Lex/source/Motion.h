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
	/// Gathering of bones(call "skeletal"). This represents a pose.
	/// </summary>
	struct Skeletal
	{
		size_t				boneCount{};
		std::vector<Bone>	skeletal{};
	};
	/// <summary>
	/// Gathering of skeletals(call "Motion"). This represents a motion(animation).
	/// </summary>
	class Motion
	{
	public:
		static constexpr float DEFAULT_SAMPLING_RATE = 1.0f / 24.0f;
	public:
		int							meshNo{};	// 0-based.
		float						samplingRate{ DEFAULT_SAMPLING_RATE };
		std::vector<std::string>	names{};
		std::vector<Skeletal>		motion{};
	};

	/// <summary>
	/// Contain a some motions.
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
		std::vector<Motion> chunk{};
		bool wasCreated{ false };
	private:
		bool Init( const std::vector<Motion> &motions );
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
		size_t   CalcMotionCount( const Motion &motion ) const;
		Skeletal FetchCurrentMotion( const Motion &motion, bool useWrapAround = true ) const;
	};
}
