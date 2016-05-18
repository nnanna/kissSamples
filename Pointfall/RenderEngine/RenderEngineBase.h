/***
Ideally handles context-switching to correct renderer
e.g OpenGl or DirectX or any else.
@TODO
***/

#pragma once
class RenderEngineBase
{
public:
	  RenderEngineBase(void);
	  ~RenderEngineBase(void);

	  void doStuff();

	  void Initialise();
};

