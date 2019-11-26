#include "Fader.h"

#include <queue>

#include "Common.h"
#include "Donya/Sprite.h"
#include "Donya/Useful.h"
#include "Donya/Vector.h"

class BaseFade
{
protected:
	enum class State
	{
		FADE_IN,
		// WAIT,
		FADE_OUT,
	};
protected:
	State	status;

	Donya::Vector2 size;	// Half-size of body.
	Donya::Vector2 pos;		// Origin is center.

	int		timer;			// 0-based.
	int		closeFrame;		// 0-based.
	bool	isClosed;		// Only completely closed.
	bool	nowHidden;		// True when completely out of screen.
public:
	BaseFade() : status( State::FADE_IN ),
		size(), pos(),
		timer( 0 ), closeFrame( 0 ),
		isClosed( false ), nowHidden( false )
	{}
	virtual ~BaseFade() = default;
public:
	/// <summary>
	/// Please call this immediately-after constructor.
	/// </summary>
	virtual void Init( int wholeCloseFrame )	= 0;
	virtual void Update()	= 0;
	virtual void Draw()		= 0;
public:
	/// <summary>
	/// Returns true only completely closed.
	/// </summary>
	virtual bool IsClosed()  const { return isClosed;  }
	/// <summary>
	/// Returns true when completely out of screen.
	/// </summary>
	virtual bool NowHidden() const { return nowHidden; }
protected:
	void SwitchFlagsByTimer()
	{
		isClosed = ( timer == 0 ) ? true : false;

		if ( timer <= -closeFrame )
		{
			nowHidden = true;
		}
	}
};

class ScrollFade : public BaseFade
{
private:
	Donya::Vector2 velocity;
public:
	ScrollFade( Donya::Vector2 direction ) : BaseFade(),
		velocity( direction )
	{}
public:
	void Init( int wholeCloseFrame ) override
	{
		size = Donya::Vector2{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		pos  = Donya::Vector2{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		timer		= wholeCloseFrame;
		closeFrame	= wholeCloseFrame;

		Donya::Vector2 destination = pos;

		pos.x += ( size.x * 2 ) * ( Donya::SignBit( velocity.x ) * -1/*Adjust pos direction is inverse of velocity.*/ );
		pos.y += ( size.y * 2 ) * ( Donya::SignBit( velocity.y ) * -1/*Adjust pos direction is inverse of velocity.*/ );

		Donya::Vector2 vecToDest = destination - pos;

		if ( wholeCloseFrame <= 0 )
		{
			// Prevent zero-divide.
			velocity.Normalize();
			return;
		}
		// else

		float speed = vecToDest.Length() / scast<float>( wholeCloseFrame );
		velocity.Normalize();
		velocity *= speed;
	}

	void Update() override
	{
		timer--;
		pos += velocity;

		SwitchFlagsByTimer();
	}

	void Draw() override
	{
		Donya::Sprite::DrawRect
		(
			pos.x, pos.y,
			size.x * 2,
			size.y * 2,
			Donya::Color::Code::BLACK, 1.0f
		);
	}
};

class GraduallyFade : public BaseFade
{
private:
	float			alpha;		// 0.0f ~ 1.0f.
	float			fadeSpeed;
	unsigned int	color;		// ARGB.
public:
	GraduallyFade( unsigned int color ) : BaseFade(),
		alpha( 0.0f ), fadeSpeed( 0.0f ),
		color( color )
	{
		
	}
public:
	void Init( int wholeCloseFrame ) override
	{
		size = Donya::Vector2{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		pos  = Donya::Vector2{ Common::HalfScreenWidthF(), Common::HalfScreenHeightF() };
		timer		= wholeCloseFrame;
		closeFrame	= wholeCloseFrame;

		if ( wholeCloseFrame <= 0 )
		{
			// Prevent zero-divide and minus value.

			fadeSpeed = 0.1f;
			return;
		}
		// else
		
		fadeSpeed = 1.0f / scast<float>( wholeCloseFrame );
	}

	void Update() override
	{
		timer--;
		isClosed = false;

		switch ( status )
		{
		case BaseFade::State::FADE_IN:
			alpha += fadeSpeed;
			if ( 1.0f <= alpha )
			{
				alpha = 1.0f;

				isClosed = true;
				status = State::FADE_OUT;
			}
			break;
		case BaseFade::State::FADE_OUT:
			alpha -= fadeSpeed;
			if ( alpha <= 0.0f )
			{
				alpha = 0.0f;
				nowHidden = true;
			}
			break;
		default: break;
		}
	}

	void Draw() override
	{
		Donya::Sprite::DrawRect
		(
			pos.x, pos.y,
			size.x * 2,
			size.y * 2,
			scast<Donya::Color::Code>( color ), alpha
		);
	}
};

void Fader::Configuration::SetDefault( Type fadeType )
{
	constexpr int DEFAULT_CLOSE_FRAME = 45;

	switch ( fadeType )
	{
	case Fader::Type::Scroll:
		type = fadeType;
		closeFrame	= DEFAULT_CLOSE_FRAME;
		parameter	= Direction::LEFT;
		break;
	case Fader::Type::Gradually:
		type = fadeType;
		closeFrame	= DEFAULT_CLOSE_FRAME;
		parameter	= scast<unsigned int>( Donya::Color::Code::BLACK );
		break;
	default: break;
	}
}
void Fader::Configuration::SetDirection( Direction dir )
{
	parameter = dir;

	NormalizeDirection();
}
void Fader::Configuration::SetDirection( Direction dirX, Direction dirY )
{
	SetDirection( scast<Direction>( scast<int>( dirX ) | scast<int>( dirY ) ) );
}
void Fader::Configuration::NormalizeDirection()
{
	unsigned int &dir = parameter;
	if ( dir & UP   && dir & DOWN  )
	{
		dir &= ~scast<int>(  DOWN  );
	}
	if ( dir & LEFT && dir & RIGHT )
	{
		dir &= ~scast<int>(  RIGHT );
	}
}
Fader::Configuration Fader::Configuration::UseDefault( Type fadeType )
{
	Configuration def{};
	def.SetDefault( fadeType );
	return def;
}

struct Fader::Impl
{
public:
	std::unique_ptr<BaseFade> pFade;
	std::queue<Configuration> reserves;
public:
	Impl() : pFade( nullptr ), reserves()
	{

	}
	~Impl()
	{
		pFade.reset( nullptr );

		// Clear idiom of std::queue.
		std::queue<Configuration>().swap( reserves );
	}
public:
	void Init()
	{
		pFade.reset( nullptr );
	}

	void Update()
	{
		if ( !pFade ) { return; }
		// else

		pFade->Update();

		if ( pFade->NowHidden() )
		{
			pFade.reset( nullptr );
		}
	}

	void Draw()
	{
		if ( !pFade ) { return; }
		// else

		pFade->Draw();
	}
public:
	void StartFade( Configuration config )
	{
		auto MakeVector = []( Configuration config )->Donya::Vector2
		{
			config.NormalizeDirection();
			unsigned int dir = config.parameter;

			Donya::Vector2 vector{};
			if ( dir & UP    ) { vector.y -= 1.0f; }
			if ( dir & DOWN  ) { vector.y += 1.0f; }
			if ( dir & LEFT  ) { vector.x -= 1.0f; }
			if ( dir & RIGHT ) { vector.x += 1.0f; }
			vector.Normalize();
			return vector;
		};

		switch ( config.type )
		{
		case Fader::Type::Scroll:
			{
				Donya::Vector2 vector = MakeVector( config );
				pFade = std::make_unique<ScrollFade>( vector );
			}
			break;
		case Fader::Type::Gradually:
			{
				pFade = std::make_unique<GraduallyFade>( config.parameter );
			}
			break;
		default: return;
		}

		pFade->Init( config.closeFrame );
	}
};

Fader::Fader() : pImpl( std::make_unique<Fader::Impl>() )
{

}
Fader::~Fader()
{
	pImpl.reset( nullptr );
}

void Fader::Init()
{
	pImpl->Init();
}

void Fader::Update()
{
	if ( !pImpl->pFade && !pImpl->reserves.empty() )
	{
		pImpl->StartFade( pImpl->reserves.front() );
		pImpl->reserves.pop();
	}

	pImpl->Update();
}

void Fader::Draw()
{
	pImpl->Draw();
}

void Fader::StartFadeOut( Configuration config, AssignmentOption option )
{
	if ( IsExist() )
	{
		switch ( option )
		{
		case Fader::AssignmentOption::Nothing:
			return; // break;
		case Fader::AssignmentOption::Overwrite:
			break;
		case Fader::AssignmentOption::Reserve:
			pImpl->reserves.push( config );
			return; // break;
		default:
			return; // break;
		}
	}

	pImpl->StartFade( config );
}

bool Fader::IsClosed() const
{
	return ( pImpl->pFade ) ? pImpl->pFade->IsClosed() : false;
}

bool Fader::IsExist() const
{
	return ( pImpl->pFade || !pImpl->reserves.empty() ) ? true : false;
}
