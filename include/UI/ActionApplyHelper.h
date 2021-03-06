#pragma once

#include <functional>

#include "state/State.h"
#include "FlowControl/FlowController.h"
#include "mcts/board/BoardActionAnalyzer.h"

namespace ui
{
	class ActionApplyHelper
	{
	public:
		struct NullInfo {};
		struct ChooseHandCardInfo {
			ChooseHandCardInfo(std::vector<size_t> const& cards) : cards(cards) {}
			std::vector<size_t> cards;
		};
		struct MinionPutLocationInfo {
			MinionPutLocationInfo(int minions) : minions(minions) {}
			int minions;
		};
		struct ChooseAttackerInfo {
			ChooseAttackerInfo(std::vector<int> targets) : targets(targets) {}
			std::vector<int> targets;
		};
		struct ChooseDefenderInfo {
			ChooseDefenderInfo(std::vector<state::CardRef> targets) : targets(targets) {}
			std::vector<state::CardRef> targets;
		};
		struct GetSpecifiedTargetInfo {
			GetSpecifiedTargetInfo(state::CardRef card_ref, std::vector<state::CardRef> targets) :
				card_ref(card_ref), targets(targets)
			{}
			state::CardRef card_ref;
			std::vector<state::CardRef> targets;
		};
		struct ChooseOneInfo {
			ChooseOneInfo(std::vector<Cards::CardId> const& cards) : cards(cards) {}
			std::vector<Cards::CardId> cards;
		};
		struct ChooseRandomInfo {
			ChooseRandomInfo(int exclusive_max) : exclusive_max(exclusive_max) {}
			int exclusive_max;
		};
		using CallbackInfo = std::variant<
			NullInfo, // no callback called
			ChooseHandCardInfo,
			MinionPutLocationInfo,
			ChooseAttackerInfo,
			ChooseDefenderInfo,
			GetSpecifiedTargetInfo,
			ChooseOneInfo>;

		using StartBoardGetter = std::function<state::State(int)>;

	private:
		class RandomCallback : public mcts::board::IRandomGenerator {
		public:
			int Get(int exclusive_max) { return 0; }
		};

		class ActionParameterCallback : public mcts::board::IRawActionParameterGetter {
		public:
			ActionParameterCallback(CallbackInfo & result, std::vector<int> const& choices, size_t & choices_idx) : 
				result_(result), choices_(choices), choices_idx_(choices_idx)
			{}

			int ChooseHandCard(std::vector<size_t> const& playable_cards) {
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					assert(playable_cards.size() > 1);
					result_ = ChooseHandCardInfo(playable_cards);
				}
				return GetNextChoice(0, (int)playable_cards.size());
			}

			int GetMinionPutLocation(int minions) {
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					assert(minions > 0);
					result_ = MinionPutLocationInfo(minions);
				}
				return GetNextChoice(0, minions+1);
			}

			int	ChooseAttacker(std::vector<int> const& attackers) {
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					assert(attackers.size() > 1);
					result_ = ChooseAttackerInfo(attackers);
				}
				return GetNextChoice(0, (int)attackers.size());
			}

			state::CardRef GetDefender(std::vector<state::CardRef> const& targets) {
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					assert(targets.size() > 1);
					result_ = ChooseDefenderInfo(targets);
				}
				int choice = GetNextChoice(0, (int)targets.size());
				return targets[choice];
			}

			state::CardRef GetSpecifiedTarget(
				state::State & state, state::CardRef card_ref,
				std::vector<state::CardRef> const& targets)
			{
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					assert(targets.size() > 1);
					result_ = GetSpecifiedTargetInfo(card_ref, targets);
				}
				int choice = GetNextChoice(0, (int)targets.size());
				return targets[choice];
			}

			Cards::CardId ChooseOne(std::vector<Cards::CardId> const& cards)
			{
				if (choices_idx_ >= choices_.size() &&
					std::holds_alternative<NullInfo>(result_))
				{
					result_ = ChooseOneInfo(cards);
				}
				int choice = GetNextChoice(0, (int)cards.size());
				return cards[choice];
			}

		private:
			int GetNextChoice(int min, int exclusive_max) {
				if (choices_idx_ >= choices_.size()) {
					return min;
				}

				int choice = choices_[choices_idx_];
				++choices_idx_;
				(void)exclusive_max;
				assert(choice >= min);
				assert(choice < exclusive_max);
				return choice;
			}

		private:
			CallbackInfo & result_;
			std::vector<int> const& choices_;
			size_t & choices_idx_;
		};

	public:
		ActionApplyHelper() : choices_() {}

		void AppendChoice(int choice) {
			choices_.push_back(choice);
		}

		void ClearChoices() {
			choices_.clear();
		}
		
		template <class StartBoardGetter>
		CallbackInfo ApplyChoices(StartBoardGetter && start_board_getter) const
		{
			state::State game_state = start_board_getter();
			return ApplyChoices(game_state);
		}

		CallbackInfo ApplyChoices(state::State & game_state) const
		{
			CallbackInfo info = NullInfo();

			size_t choices_idx = 0;

			RandomCallback rand_cb;
			ActionParameterCallback action_cb(info, choices_, choices_idx);
			FlowControl::FlowContext flow_context;
			flow_context.SetCallback(rand_cb, action_cb);

			while (choices_.size() > choices_idx) {
				int main_op = choices_[choices_idx];
				++choices_idx;

				mcts::board::BoardActionAnalyzer analyzer;
				FlowControl::CurrentPlayerStateView state_view(game_state);
				analyzer.GetActionsCount(state_view);
				analyzer.ApplyAction(flow_context, state_view, main_op, rand_cb, action_cb);
			}

			return info;
		}

	private:
		std::vector<int> choices_;
	};
}