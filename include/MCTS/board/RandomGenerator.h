#pragma once

#include "state/IRandomGenerator.h"

namespace mcts
{
	class SOMCTS;

	namespace board
	{
		class IRandomGenerator : public state::IRandomGenerator {
		public:
			size_t Get(size_t exclusive_max) final
			{
				return (size_t)Get((int)exclusive_max);
			}

			// @param min Inclusive minimum
			// @param max Inclusive maximum
			int Get(int min, int max) final
			{
				assert(max >= min);
				return min + Get(max - min + 1);
			}

			virtual int Get(int exclusive_max) = 0;
		};

		class RandomGenerator : public IRandomGenerator
		{
		public:
			RandomGenerator(SOMCTS & callback) : callback_(callback) {}

			int Get(int exclusive_max) final;

		private:
			SOMCTS & callback_;
		};
	}
}