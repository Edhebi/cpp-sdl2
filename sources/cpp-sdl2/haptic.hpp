#pragma once

#include <SDL_haptic.h>
#include <algorithm>

namespace sdl
{
class Haptic
{
	///Pointer to C sdl haptic device
	SDL_Haptic* haptic_ = nullptr;

	///Type of an installed "effect" for SDL
	using effect_sdlid = int;

	///Installed effect storage type
	using effect_list  = std::vector<effect_sdlid>;

	///Installed effect storage
	effect_list my_effects{};

	///Move utility for move ctor/assign operations
	void move_from(Haptic& other)
	{
		haptic_		  = other.haptic_;
		my_effects	  = std::move(other.my_effects);
		other.haptic_ = nullptr;
	}

public:
	///The the C pointer
	SDL_Haptic* ptr() const { return haptic_; }

	///Installed effect handle
	class InstalledEffect
	{
		effect_list::size_type index_ = ~0ULL;
		Haptic*				   owner_ = nullptr;
		friend class Haptic;

		void move_from(InstalledEffect& other)
		{
			index_ = other.index_;
			owner_ = other.owner_;

			other.index_ = ~0ULL;
			other.owner_ = nullptr;
		}

	public:
		InstalledEffect(effect_list::size_type index, Haptic* owner)
			: index_{index}, owner_{owner}
		{
		}

		InstalledEffect() {}
		InstalledEffect(InstalledEffect const&);
		InstalledEffect& operator=(InstalledEffect const&);

		InstalledEffect(InstalledEffect&& other) { move_from(other); }

		InstalledEffect& operator=(InstalledEffect&& other)
		{
			move_from(other);
			return *this;
		}

		~InstalledEffect()
		{
			if (owner_ && index_ != ~0ULL)
			{
				const effect_sdlid my_real_id = owner_->get_effect_sdlid(*this);
				SDL_HapticDestroyEffect(owner_->ptr(), my_real_id);

				owner_->remove_effect(my_real_id);
			}
		}

		void run(uint32_t iterations = 1)
		{
			owner_->run_effect(*this, iterations);
		}
	};

	///Effect defintion
	union Effect
	{
		// note : It has been planned by SDL developers to change the `type`
		// type in 2.1.0 This is analog to how we handle sdl events, this
		// definition is copied from SDL_haptic.h
		Uint16				type;	   /// Effect type.
		SDL_HapticConstant	constant;  /// Constant effect.
		SDL_HapticPeriodic	periodic;  /// Periodic effect.
		SDL_HapticCondition condition; /// Condition effect.
		SDL_HapticRamp		ramp;	   /// Ramp effect.
		SDL_HapticLeftRight leftright; /// Left/Right effect.
		SDL_HapticCustom	custom;	   /// Custom effect.

		///this permit to treat an sdl::haptic::effect instance as if it was an SDL_HapticEffect to it's own content
		operator SDL_HapticEffect*() const
		{
			//"I solemnly swear that I am up to no good."
			return (SDL_HapticEffect*)(this);
		}

		///Construct the effect. Fill it with zeroes
		Effect()
		{
			// be sure to fully zero initialize the enclosed effect
			SDL_memset(this, 0, sizeof(Effect));
		}
	};

	///Uninitialized dummy haptic device
	Haptic() {}

	///Open haptic device from index
	Haptic(int haptic_index) : haptic_{SDL_HapticOpen(haptic_index)}
	{
		if (!haptic_)
		{
			throw Exception("SDL_HapticOpen");
		}
	}

	///Open haptic device from joystick pointer
	Haptic(SDL_Joystick* joystick)
		: haptic_{SDL_HapticOpenFromJoystick(joystick)}
	{
		if (!haptic_)
		{
			throw Exception("SDL_HapticOpenFromJoystick");
		}
	}

	///close the haptic device automatically
	~Haptic()
	{
		if (haptic_)
		{
			SDL_HapticClose(haptic_);
		}
	}

	//not copyable
	Haptic(Haptic const&) = delete;
	Haptic& operator=(Haptic const&) = delete;

	///move ctor
	Haptic(Haptic&& other) { move_from(other); }

	///move assign opeartor
	Haptic& operator=(Haptic&& other)
	{
		move_from(other);
		return *this;
	}

	///Return true if the device is correctly opened
	bool valid() const { return haptic_ != nullptr; }

	///Get a bitflag that describe the device capabilities
	unsigned int get_capabilities() const
	{
		if (!valid()) // we're not an opened device... So we should decide what
					  // to do here...
			return 0U;

		const auto capabilities = SDL_HapticQuery(haptic_);

		if (!capabilities)
		{
			// couldn't get the capabilities of this haptic device.
			throw Exception("SDL_HapticQuery");
		}

		return capabilities;
	}

	///Check the SDL_HAPTIC_ flag agianst the device capabilities
	bool is_capable_of(int haptic_flag) const
	{
		const auto caps = get_capabilities();
		return (haptic_flag & caps) != 0;
	}

	///Install the effect
	InstalledEffect new_effect(Effect const& e)
	{
		// We need to be able to play the haptic effect
		if (!is_capable_of(e.type)) return {};

		// maybe we should have another RAII wrapper around the registered
		// haptic effects
		const effect_sdlid raw_sdl_id = SDL_HapticNewEffect(haptic_, e);

		if (raw_sdl_id < 0) throw Exception("SDL_HapticNewEffect");

		my_effects.push_back(raw_sdl_id);
		return {my_effects.size() - 1, this};
	}

	///Get the number of effects installed
	effect_list::size_type registered_effect_count() const
	{
		return my_effects.size();
	}

	///Get the SDL assigned ID (an integer) to the effect
	effect_sdlid get_effect_sdlid(InstalledEffect const& h) const
	{
		return my_effects.at(h.index_);
	}

	///Deactiave the effect. This only set the registered id in question to -1 
	void remove_effect(effect_sdlid e)
	{
		for(int i = 0, nb_effects = my_effects.size(); i < nb_effects; ++i)
		{
			if (my_effects[i] == e) my_effects[i] = -1;
		}
	}

	///Run an effect, if said effect is valid
	void run_effect(InstalledEffect const& h, uint32_t iterations = 1) const
	{
		const effect_sdlid e = get_effect_sdlid(h);
		if (e >= 0 && SDL_HapticRunEffect(haptic_, get_effect_sdlid(h), iterations) < 0)
		{
			throw Exception("SDL_HapticRunEffect");
		}
	}

	///Check if you can safely attemp to install the effect ont he haptic device
	bool is_effect_compatible(Effect const& e) const
	{
		return is_capable_of(e.type);
	}
};
} // namespace sdl
