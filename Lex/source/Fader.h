#pragma once

#include <memory>

#include "Donya/Constant.h"
#include "Donya/Template.h"

/// <summary>
/// TODO : Change to the user can specify drawing sprite or color.
/// </summary>
class Fader final : public Donya::Singleton<Fader>
{
	friend class Donya::Singleton<Fader>;
public:
	/// <summary>
	/// You can specify the type of fade-out, fade-in.
	/// </summary>
	enum class Type
	{
		Scroll,		// 
		Gradually,	// 
	};
	/// <summary>
	/// If set [DOWN|RIGHT] to parameter, the fade will move to right-bottom from left-top.<para></para>
	/// If set [UP|DOWN] or [LEFT|RIGHT], will UP or LEFT prioritize.
	/// </summary>
	enum Direction
	{
		UP		= 1 << 0,
		DOWN	= 1 << 1,
		LEFT	= 1 << 2,
		RIGHT	= 1 << 3,
	};

	/// <summary>
	/// "type" : You specify type of fade.<para></para>
	/// "closeFrame" : You specify time of completely close(per frame).<para></para>
	/// "parameter" :<para></para>
	/// Type::Scroll : Used to judge direction(you can specify by Fader::Direction).<para></para>
	/// Type::Gradually : Used to fill color. This is linking to Donya::Color::Code.
	/// </summary>
	struct Configuration
	{
		Type			type{};			// You specify type of fade.
		int				closeFrame{};	// You specify time of completely close(per frame)
		unsigned int	parameter{};	// [Type::Scroll : Used to judge direction(you can specify by Fader::Direction)] [Type::Gradually : Used to fill color. This is linking to Donya::Color::Code]
	public:
		void SetDefault( Type fadeType );
		void SetDirection( Direction moveDirection );
		void SetDirection( Direction dirX, Direction dirY );
		void NormalizeDirection();
	public:
		static Configuration UseDefault( Type fadeType );
	};

	/// <summary>
	/// This option used in when if not completed current fade process.
	/// </summary>
	enum class AssignmentOption
	{
		Nothing,	// Doing nothing.
		Overwrite,	// Overwrite by the new fade. the old fade will be dispose immediately.
		Reserve,	// Reserved fade will apply when finished current fade.
	};
private:
	struct Impl;
	std::unique_ptr<Impl> pImpl;
private:
	Fader();
public:
	~Fader();
public:
	/// <summary>
	/// Initialize and reset current state.
	/// </summary>
	void Init();

	/// <summary>
	/// Please call every frame.
	/// </summary>
	void Update();

	void Draw();
public:
	/// <summary>
	/// "config" : Please set configuration of fade.<para></para>
	/// "option" : You can choose behavior when if the current fade is not complete.
	/// </summary>
	void StartFadeOut( Configuration config, AssignmentOption option = AssignmentOption::Reserve );
public:
	/// <summary>
	/// Returns true if only when the fade is completely closed.
	/// </summary>
	bool IsClosed() const;
	/// <summary>
	/// Returns true when exist instance even one.
	/// </summary>
	bool IsExist() const;
};
