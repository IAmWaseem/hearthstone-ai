#pragma once

#include "state/IRandomGenerator.h"
#include "state/Cards/CardData.h"
#include "FlowControl/FlowContext.h"
#include "Cards/id-map.h"

namespace state
{
	class State;
}

namespace FlowControl
{
	namespace Manipulators
	{
		class BoardManipulator
		{
		public:
			BoardManipulator(state::State & state, FlowContext & flow_context)
				: state_(state), flow_context_(flow_context)
			{
			}

		public: // bridge to state::State
			state::Cards::Card const& GetCard(state::CardRef ref);
			state::PlayerIdentifier GetCurrentPlayerId();
			int GetTurn() const;

			state::board::Player & Player(state::PlayerIdentifier player);
			state::board::Player const& Player(state::PlayerIdentifier player) const;
			state::board::Player & FirstPlayer() { return Player(state::PlayerIdentifier::First()); }
			state::board::Player const& FirstPlayer() const { return Player(state::PlayerIdentifier::First()); }
			state::board::Player & SecondPlayer() { return Player(state::PlayerIdentifier::Second()); }
			state::board::Player const& SecondPlayer() const { return Player(state::PlayerIdentifier::Second()); }
			
		public:
			state::CardRef AddCardById(Cards::CardId card_id, state::PlayerIdentifier player);
			state::CardRef AddCardByCopy(state::Cards::Card const& card, state::PlayerIdentifier player);

			state::CardRef SummonMinionById(Cards::CardId card_id, state::PlayerIdentifier player, int pos) {
				return SummonMinion(AddCardById(card_id, player), pos);
			}
			state::CardRef SummonMinionByCopy(state::Cards::Card const& card, state::PlayerIdentifier player, int pos) {
				return SummonMinion(AddCardByCopy(card, player), pos);
			}

			int GetSpellDamage(state::PlayerIdentifier player);

			void CalculateFinalDamageAmount(state::CardRef source, int amount, int * final_amount);

		private:
			state::Cards::Card GenerateCard(state::Cards::CardData card_data, state::PlayerIdentifier player);

			state::CardRef SummonMinion(state::CardRef card_ref, int pos);

		private:
			state::State & state_;
			FlowContext & flow_context_;
		};
	}
}