#pragma once

#include <memory>

#include "Scene.h"

/// <summary>
/// For another project's scene test.
/// </summary>
class SceneLex : public Scene
{
public:
	struct Impl;
private:
	std::unique_ptr<Impl> pImpl;
public:
	SceneLex();
	~SceneLex();
public:
	void	Init() override;
	void	Uninit() override;

	Result	Update( float elapsedTime ) override;

	void	Draw( float elapsedTime ) override;
private:
	Result	ReturnResult();
};
