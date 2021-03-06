#pragma once

#include <random>
#include <sstream>
#include <string>
#include <shared_mutex>

#include <json/json.h>

#include "state/Configs.h"
#include "Cards/Database.h"
#include "UI/AIController.h"
#include "UI/Decks.h"
#include "UI/Board/Board.h"
#include "UI/GameEngineLogger.h"
#include "UI/Board/UnknownCards.h"
#include "MCTS/TestStateBuilder.h"

namespace ui
{
	namespace board
	{
		// Thread safety: No
		class Parser
		{
		private:
			class MyRandomGenerator : public state::IRandomGenerator
			{
			public:
				MyRandomGenerator(int seed) : random_(seed) {}

				int Get(int exclusive_max)
				{
					return random_() % exclusive_max;
				}

				size_t Get(size_t exclusive_max) { return (size_t)Get((int)exclusive_max); }

				int Get(int min, int max)
				{
					return min + Get(max - min + 1);
				}

			public:
				std::mt19937 random_;
			};

		public:
			static constexpr int kDefaultRootSampleCount = 100;

			Parser(GameEngineLogger & logger) :
				logger_(logger), board_(),
				root_sample_count_(kDefaultRootSampleCount)
			{}

			// @note Should be set before running
			void SetRootSampleCount(int v) {
				root_sample_count_ = v;
			}
			
			int ChangeBoard(std::string const& board_raw, std::mt19937 & rand)
			{
				Json::Reader reader;
				Json::Value json_board;
				std::stringstream ss(board_raw);
				if (!reader.parse(ss, json_board)) {
					logger_.Log("Failed to parse board.");
					return -1;
				}

				int player_entity_id = json_board["player"]["entity_id"].asInt();
				int opponent_entity_id = json_board["opponent"]["entity_id"].asInt();

				board_.Reset();

				//std::string player_deck_type = "InnKeeperBasicMage"; // TODO: use correct deck
				std::string player_deck_type = "InnKeeperExpertWarlock"; // TODO: use correct deck
				std::vector<Cards::CardId> player_deck_cards;
				for (auto const& card_name : Decks::GetDeck(player_deck_type)) {
					Cards::CardId card_id = (Cards::CardId)Cards::Database::GetInstance().GetIdByCardName(card_name);
					player_deck_cards.push_back(card_id);
				}
				// TODO: remove revealed deck cards
				board_.SetDeckCards(1, player_deck_cards);

				// TODO: guess oppoennt deck type
				//std::string opponent_deck_type = "InnKeeperBasicMage";
				std::string opponent_deck_type = "InnKeeperExpertWarlock"; // TODO: use correct deck
				std::vector<Cards::CardId> opponent_deck_cards;
				for (auto const& card_name : Decks::GetDeck(opponent_deck_type)) {
					Cards::CardId card_id = (Cards::CardId)Cards::Database::GetInstance().GetIdByCardName(card_name);
					opponent_deck_cards.push_back(card_id);
				}
				board_.SetDeckCards(2, opponent_deck_cards);

				board_.Parse(json_board);

				PrepareRootSamples(rand);

				return 0;
			}

			state::State GetStartBoard(int seed)
			{
				state::State state;
				std::mt19937 rand(seed);

				int root_sample_idx = rand() % root_sample_count_;

				MyRandomGenerator random(rand());
				MakePlayer(state::kPlayerFirst, state, random, board_.GetFirstPlayer(), first_unknown_cards_sets_mgrs_[root_sample_idx]);
				MakePlayer(state::kPlayerSecond, state, random, board_.GetSecondPlayer(), second_unknown_cards_sets_mgrs_[root_sample_idx]);
				state.GetMutableCurrentPlayerId().SetFirst(); // AI is helping first player, and should now waiting for an action
				state.SetTurn(board_.GetTurn());

				return state;
			}

		private:
			void PrepareRootSamples(std::mt19937 & rand)
			{
				if (first_unknown_cards_sets_mgrs_.size() != root_sample_count_) {
					first_unknown_cards_sets_mgrs_.clear();
					second_unknown_cards_sets_mgrs_.clear();

					for (int i = 0; i < root_sample_count_; ++i) {
						first_unknown_cards_sets_mgrs_.emplace_back(board_.GetUnknownCardsSets(1));
						second_unknown_cards_sets_mgrs_.emplace_back(board_.GetUnknownCardsSets(2));
					}
				}

				for (int i = 0; i < root_sample_count_; ++i) {
					first_unknown_cards_sets_mgrs_[i].Prepare(rand);
					second_unknown_cards_sets_mgrs_[i].Prepare(rand);
				}
			}

			Cards::CardId GetCardId(std::string const& card_id)
			{
				auto const& container = Cards::Database::GetInstance().GetIdMap();
				auto it = container.find(card_id);
				if (it == container.end()) {
					return Cards::kInvalidCardId;
				}
				return (Cards::CardId)it->second;
			}

			void MakePlayer(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random,
				board::Player const& board_player, board::UnknownCardsSetsManager const& unknown_cards_sets_mgr)
			{
				MakeHero(player, state, board_player.hero);
				MakeHeroPower(player, state, board_player.hero_power);
				MakeDeck(player, state, random, board_player.deck, unknown_cards_sets_mgr);
				MakeHand(player, state, random, board_player.hand, unknown_cards_sets_mgr);
				MakeMinions(player, state, random, board_player.minions);
				// TODO: enchantments
				// TODO: weapon
				// TODO: secrets

				state.GetBoard().Get(player).SetFatigueDamage(board_player.fatigue);
				MakeResource(state.GetBoard().Get(player).GetResource(), board_player.resource);
			}

			void ApplyStatus(state::Cards::CardData & raw_card, board::CharacterStatus const& status)
			{
				raw_card.enchanted_states.charge = status.charge;
				raw_card.taunt = status.taunt;
				raw_card.shielded = status.divine_shield;
				raw_card.enchanted_states.stealth = status.stealth;
				// TODO: forgetful
				raw_card.enchanted_states.freeze_attack = status.freeze;
				raw_card.freezed = status.frozon;
				raw_card.enchanted_states.poisonous = status.poisonous;
				raw_card.enchanted_states.stealth = status.stealth;

				int max_attacks_per_turn = 1;
				if (status.windfury) max_attacks_per_turn = 2;
				raw_card.enchanted_states.max_attacks_per_turn = max_attacks_per_turn;
			}

			void AddMinion(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random, board::Minion const& minion, int pos)
			{
				state::Cards::CardData raw_card = Cards::CardDispatcher::CreateInstance(minion.card_id);
				raw_card.enchanted_states.player = player;
				raw_card.enchantment_handler.SetOriginalStates(raw_card.enchanted_states);

				raw_card.enchanted_states.max_hp = minion.max_hp;
				raw_card.damaged = minion.damage;
				raw_card.enchanted_states.attack = minion.attack;
				raw_card.num_attacks_this_turn = minion.attacks_this_turn;
				// TODO: exhausted (needed?)
				raw_card.silenced = minion.silenced;
				raw_card.enchanted_states.spell_damage = minion.spellpower;
				// TODO: enchantments
				ApplyStatus(raw_card, minion.status);

				raw_card.zone = state::kCardZoneNewlyCreated;
				raw_card.enchantment_handler.SetOriginalStates(raw_card.enchanted_states); // TODO: apply a enchantment to make up the states
				state::CardRef ref = state.AddCard(state::Cards::Card(raw_card));
				state.GetZoneChanger<state::kCardTypeMinion, state::kCardZoneNewlyCreated>(ref)
					.ChangeTo<state::kCardZonePlay>(player, pos);

				// TODO:
				// Check stats changed after put in board
				// Stats might changed due to some triggers, e.g., just_played flag
				state.GetMutableCard(ref).SetJustPlayedFlag(minion.summoned_this_turn);
				state.GetMutableCard(ref).SetNumAttacksThisTurn(minion.attacks_this_turn);
			}

			void MakeMinions(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random, board::Minions const& minions)
			{
				int pos = 0;
				for (auto const& minion : minions.minions) {
					AddMinion(player, state, random, minion, pos);
					++pos;
				}
			}

			void MakeResource(state::board::PlayerResource & resource, board::Resource const& board_resource)
			{
				resource.SetCurrent(board_resource.current);
				resource.SetTotal(board_resource.total);
				resource.SetCurrentOverloaded(board_resource.overload);
				resource.SetNextOverload(board_resource.overload_next_turn);
			}

			void MakeHero(state::PlayerIdentifier player, state::State & state, board::Hero const& board_hero)
			{
				// TODO: mark ctor of state::Cards::CardData as private, to make it only constructible from CardDispatcher::CreateInstance

				state::Cards::CardData raw_card = Cards::CardDispatcher::CreateInstance(board_hero.card_id);

				raw_card.enchanted_states.player = player;
				assert(raw_card.card_type == state::kCardTypeHero);
				raw_card.zone = state::kCardZoneNewlyCreated;
				raw_card.enchantment_handler.SetOriginalStates(raw_card.enchanted_states);

				raw_card.enchanted_states.max_hp = board_hero.max_hp;
				raw_card.damaged = board_hero.damage;
				raw_card.armor = board_hero.armor;
				raw_card.enchanted_states.attack = board_hero.attack;
				ApplyStatus(raw_card, board_hero.status);

				// TODO: enchantments

				raw_card.enchantment_handler.SetOriginalStates(raw_card.enchanted_states);
				state::CardRef ref = state.AddCard(state::Cards::Card(raw_card));
				state.GetZoneChanger<state::kCardTypeHero, state::kCardZoneNewlyCreated>(ref)
					.ChangeTo<state::kCardZonePlay>(player);

				state.GetMutableCard(ref).SetNumAttacksThisTurn(board_hero.attacks_this_turn);
			}

			void MakeHeroPower(state::PlayerIdentifier player, state::State & state, board::HeroPower const& hero_power)
			{
				state::Cards::CardData raw_card = Cards::CardDispatcher::CreateInstance(hero_power.card_id);
				assert(raw_card.card_type == state::kCardTypeHeroPower);

				raw_card.zone = state::kCardZoneNewlyCreated;
				state::CardRef ref = state.AddCard(state::Cards::Card(raw_card));
				state.GetZoneChanger<state::kCardTypeHeroPower, state::kCardZoneNewlyCreated>(ref)
					.ChangeTo<state::kCardZonePlay>(player);

				state.GetMutableCard(ref).SetUsable(!hero_power.used);
			}

			void PushBackDeckCard(Cards::CardId id, state::IRandomGenerator & random, state::State & state, state::PlayerIdentifier player)
			{
				int deck_count = (int)state.GetBoard().Get(player).deck_.Size();
				state.GetBoard().Get(player).deck_.ShuffleAdd(id, random);
				++deck_count;
				assert(state.GetBoard().Get(player).deck_.Size() == deck_count);
			}

			void MakeDeck(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random, std::vector<int> entities,
				board::UnknownCardsSetsManager const& unknown_cards_sets_mgr)
			{
				for (int entity_id : entities) {
					Cards::CardId card_id = board_.GetCardId(entity_id, unknown_cards_sets_mgr);
					PushBackDeckCard(card_id, random, state, player);
				}
			}

			state::CardRef AddHandCard(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random, Cards::CardId card_id)
			{
				state::Cards::CardData raw_card = Cards::CardDispatcher::CreateInstance(card_id);
				raw_card.enchanted_states.player = player;
				raw_card.enchantment_handler.SetOriginalStates(raw_card.enchanted_states);
				raw_card.zone = state::kCardZoneNewlyCreated;

				auto ref = state.AddCard(state::Cards::Card(raw_card));

				int hand_count = (int)state.GetBoard().Get(player).hand_.Size();
				state.GetZoneChanger<state::kCardZoneNewlyCreated>(ref)
					.ChangeTo<state::kCardZoneHand>(player);

				assert(state.GetCardsManager().Get(ref).GetCardId() == card_id);
				assert(state.GetCardsManager().Get(ref).GetPlayerIdentifier() == player);
				if (hand_count == 10) {
					assert(state.GetBoard().Get(player).hand_.Size() == 10);
					assert(state.GetCardsManager().Get(ref).GetZone() == state::kCardZoneGraveyard);
				}
				else {
					++hand_count;
					assert((int)state.GetBoard().Get(player).hand_.Size() == hand_count);
					assert(state.GetCardsManager().Get(ref).GetZone() == state::kCardZoneHand);
					if constexpr (!state::kOrderHandCardsByCardId) {
						assert(state.GetBoard().Get(player).hand_.Get(hand_count - 1) == ref);
						assert(state.GetCardsManager().Get(ref).GetZonePosition() == (hand_count - 1));
					}
				}

				return ref;
			}

			void MakeHand(state::PlayerIdentifier player, state::State & state, state::IRandomGenerator & random, std::vector<int> const& entities,
				board::UnknownCardsSetsManager const& unknown_cards_sets_mgr)
			{
				for (int entity_id : entities) {
					Cards::CardId card_id = board_.GetCardId(entity_id, unknown_cards_sets_mgr);
					AddHandCard(player, state, random, card_id);
				}
			}

		private:
			GameEngineLogger & logger_;
			board::Board board_;
			int root_sample_count_;
			std::vector<board::UnknownCardsSetsManager> first_unknown_cards_sets_mgrs_;
			std::vector<board::UnknownCardsSetsManager> second_unknown_cards_sets_mgrs_;
		};
	}
}