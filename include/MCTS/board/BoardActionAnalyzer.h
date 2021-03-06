#pragma once

#include <shared_mutex>
#include "MCTS/Types.h"
#include "FlowControl/PlayerStateView.h"
#include "Utils/SpinLocks.h"

namespace mcts
{
	namespace board
	{
		class IRawActionParameterGetter;
		class IRandomGenerator;

		class BoardActionAnalyzer
		{
		public:
			BoardActionAnalyzer() : mutex_(), op_map_(), op_map_size_(0), attackers_(), playable_cards_() {}

			BoardActionAnalyzer(BoardActionAnalyzer const& rhs) :
				mutex_(),
				op_map_(rhs.op_map_),
				op_map_size_(rhs.op_map_size_),
				attackers_(rhs.attackers_),
				playable_cards_(rhs.playable_cards_)
			{}

			BoardActionAnalyzer & operator=(BoardActionAnalyzer const& rhs) {
				op_map_ = rhs.op_map_;
				op_map_size_ = rhs.op_map_size_;
				attackers_ = rhs.attackers_;
				playable_cards_ = rhs.playable_cards_;
				return *this;
			}

			void Reset() { op_map_size_ = 0; }

			int GetActionsCount(FlowControl::CurrentPlayerStateView const& board);

			Result ApplyAction(
				FlowControl::FlowContext & flow_context,
				FlowControl::CurrentPlayerStateView board,
				int action,
				IRandomGenerator & random,
				IRawActionParameterGetter & action_parameters);

			enum OpType {
				kPlayCard,
				kAttack,
				kHeroPower,
				kEndTurn,
				kInvalid, kMaxOpType = kInvalid
			};

			template <class Functor>
			void ForEachMainOp(Functor && functor) const {
				std::shared_lock<Utils::SharedSpinLock> lock(mutex_);
				for (size_t i = 0; i < op_map_size_; ++i) {
					if (!functor(i, GetMainOpType(op_map_[i]))) return;
				}
			}
			OpType GetMainOpType(size_t choice) const {
				std::shared_lock<Utils::SharedSpinLock> lock(mutex_);
				return GetMainOpType(op_map_[choice]);
			}
			template <class Functor>
			void ForEachPlayableCard(Functor && functor) const {
				std::shared_lock<Utils::SharedSpinLock> lock(mutex_);
				for (auto hand_idx : playable_cards_) {
					if (!functor(hand_idx)) break;
				}
			}
			size_t GetPlaybleCard(size_t idx) const {
				std::shared_lock<Utils::SharedSpinLock> lock(mutex_);
				return playable_cards_[idx];
			}

		private:
			Result ConvertResult(FlowControl::Result flow_result) {
				// The player AI is helping is defined to be the first player
				switch (flow_result) {
				case FlowControl::Result::kResultFirstPlayerWin:
					return Result::kResultWin;

				case FlowControl::Result::kResultSecondPlayerWin:
				case FlowControl::Result::kResultDraw:
					return Result::kResultLoss;

				case FlowControl::Result::kResultNotDetermined:
					return Result::kResultNotDetermined;

				case FlowControl::Result::kResultInvalid:
				default:
					return Result::kResultInvalid;
				}
			}

			typedef Result (BoardActionAnalyzer::*OpFunc)(FlowControl::FlowContext & flow_context, FlowControl::CurrentPlayerStateView & board, IRandomGenerator & random, IRawActionParameterGetter & action_parameters);
			Result PlayCard(FlowControl::FlowContext & flow_context, FlowControl::CurrentPlayerStateView & board, IRandomGenerator & random, IRawActionParameterGetter & action_parameters);
			Result Attack(FlowControl::FlowContext & flow_context, FlowControl::CurrentPlayerStateView & board, IRandomGenerator & random, IRawActionParameterGetter & action_parameters);
			Result HeroPower(FlowControl::FlowContext & flow_context, FlowControl::CurrentPlayerStateView & board, IRandomGenerator & random, IRawActionParameterGetter & action_parameters);
			Result EndTurn(FlowControl::FlowContext & flow_context, FlowControl::CurrentPlayerStateView & board, IRandomGenerator & random, IRawActionParameterGetter & action_parameters);

			OpType GetMainOpType(OpFunc func) const {
				if (func == &BoardActionAnalyzer::PlayCard) return kPlayCard;
				if (func == &BoardActionAnalyzer::Attack) return kAttack;
				if (func == &BoardActionAnalyzer::HeroPower) return kHeroPower;
				if (func == &BoardActionAnalyzer::EndTurn) return kEndTurn;
				return kInvalid;
			}

		private:
			mutable Utils::SharedSpinLock mutex_;
			std::array<OpFunc, kMaxOpType> op_map_;
			size_t op_map_size_;
			std::vector<int> attackers_;
			std::vector<size_t> playable_cards_;
		};
	}
}