
#include "InputListener.h"
#include "GLApplication.h"
#include <GL/glut.h>


ksU32	InputListener::mKeyDown = 0;

InputListener::InputListener()
{}


InputListener::~InputListener()
{}


void InputListener::KeyDownCallback(unsigned char c, int x, int y)
{
	int mods = glutGetModifiers();

	if (mods & GLUT_ACTIVE_SHIFT)
	{
		mKeyDown |= KEYPRESS_SHIFT;
	}
	else if ((mKeyDown & KEYPRESS_SHIFT))
	{
		mKeyDown &= ~KEYPRESS_SHIFT;
	}

	switch (c)
	{		
	case 'W':
	case 'w':
		mKeyDown |= KEYPRESS_UP;
		break;

	case 'A':
	case 'a':
		mKeyDown |= KEYPRESS_LEFT;
		break;

	case 'S':
	case 's':
		mKeyDown |= KEYPRESS_DOWN;
		break;

	case 'D':
	case 'd':
		mKeyDown |= KEYPRESS_RIGHT;
		break;

	case 27:  // Esc key
		break;

	case 23:		// Ctrl+W
		mKeyDown |= KEYPRESS_UP | KEYPRESS_CTRL;
		break;

	case 19:		// Ctrl+S
		mKeyDown |= KEYPRESS_DOWN | KEYPRESS_CTRL;
		break;

	default:
		mKeyDown = KEYPRESS_NONE;
	}
}


void InputListener::KeyUpCallback(unsigned char c, int x, int y)
{
	switch (c)
	{
	case 'W':
	case 'w':
		mKeyDown &= ~KEYPRESS_UP;
		break;

	case 'A':
	case 'a':
		mKeyDown &= ~KEYPRESS_LEFT;
		break;

	case 'S':
	case 's':
		mKeyDown &= ~KEYPRESS_DOWN;
		break;

	case 'D':
	case 'd':
		mKeyDown &= ~KEYPRESS_RIGHT;
		break;

	case 23:		// Ctrl+W
		mKeyDown &= ~(KEYPRESS_UP | KEYPRESS_CTRL);
		break;

	case 19:		// Ctrl+S
		mKeyDown &= ~(KEYPRESS_DOWN | KEYPRESS_CTRL);
		break;

	case 27:  // Esc key
		GLApplication::destroy(0);
		break;
	}
	DEBUG_PRINT("keyup %c\n", c);
}


void InputListener::Poll()
{

}