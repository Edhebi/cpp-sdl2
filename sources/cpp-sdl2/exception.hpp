#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

#include <SDL_error.h>

///Define to deactivate exception support
#ifdef CPP_SDL2_DISABLE_EXCEPTIONS
#include <iostream> //Instead of throwing, we are going to dump the function and error into stderr via std::cerr
#endif

namespace sdl
{
///Define to deactivate exception support
#ifndef CPP_SDL2_DISABLE_EXCEPTIONS

///Custom exception object
class Exception : public std::exception
{
public:
	///Construct an exception object. Will get the content of SDL_GetError() ont the '.what()' message
	///\param function string containing the name of the SDL function
	Exception(std::string const& function)
		: function_{function}, error_{SDL_GetError()}
	{
		SDL_ClearError();
	}

	///Return the string containing the name of the SDL function that failed
	std::string function() { return function_; }
	///Return the error message generated by SDL_GetError() at event object construction
	std::string error() { return error_; }

	const char* what() const noexcept override
	{
		if (what_.empty())
		{
			what_ = "function: ";
			what_ += function_;
			what_ += " SDL error: ";
			what_ += error_;
		}

		return what_.c_str();
	}

private:
	///storage for function name string
	std::string function_;
	///storage for SDL_GetError() return string
	std::string error_;
	///storage for the "what()" string. string is dynamically built when calling what() const
	mutable std::string what_;
};

#else // Do not use exceptions

// Stub the 'throw' keyword
#define throw

///Castrated exception class
class Exception
{
public:
	///Constructor that will dump error informations to stderr, then kill the program
	Exception(std::string const& function)
	{
		std::cerr << "Error in : " << function << "\n";
		const auto error = SDL_GetError();
		std::cerr << "Error : " << error << "\n";
		abort();
	}
};

#endif

} // namespace sdl
