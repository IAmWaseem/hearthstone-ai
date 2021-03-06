#pragma once

// http://www.hearthpwn.com/cards?filter-set=2&filter-class=32&sort=-cost&display=1
// DONE

namespace Cards
{
	struct Card_CS2_101 : public HeroPowerCardBase<Card_CS2_101> {
		Card_CS2_101() {
			onplay_handler.SetCheckPlayableCallback([](FlowControl::onplay::context::CheckPlayable & context) {
				if (context.state_.GetBoard().Get(context.player_).minions_.Full()) return false;
				return true;
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				SummonToRightmost(context.manipulate_, context.player_, Cards::ID_CS2_101t);
			});
		}
	};

	struct Card_CS2_101_H1 : public Card_CS2_101 {};
	struct Card_CS2_101_H2 : public Card_CS2_101 {};

	struct Card_CS2_087e : public Enchantment<Card_CS2_087e, Attack<3>> {};
	struct Card_CS2_087 : public SpellCardBase<Card_CS2_087> {
		Card_CS2_087() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_).Minion();
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardMinion(target).Enchant().Add<Card_CS2_087e>();
			});
		}
	};

	struct Card_EX1_371 : public SpellCardBase<Card_EX1_371> {
		Card_EX1_371() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_).Minion();
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardMinion(target).Shield(true);
			});
		}
	};

	struct Card_EX1_360e : Enchantment<Card_EX1_360e, SetAttack<1>> {};
	struct Card_EX1_360 : SpellCardBase<Card_EX1_360> {
		Card_EX1_360() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_).Minion();
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardMinion(target).Enchant().Add<Card_EX1_360e>();
			});
		}
	};

	struct Card_CS2_089 : public SpellCardBase<Card_CS2_089> {
		Card_CS2_089() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_);
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardCharacter(target).Heal(context.card_ref_, 6);
			});
		}
	};

	struct Card_CS2_097 : public WeaponCardBase<Card_CS2_097> {
		static bool HandleEvent(state::CardRef self, state::Events::EventTypes::BeforeAttack::Context const& context) {
			state::PlayerIdentifier owner = context.manipulate_.GetCard(self).GetPlayerIdentifier();
			state::CardRef hero_ref = context.manipulate_.Board().Player(owner).GetHeroRef();
			if (context.attacker_ != hero_ref) return true;
			context.manipulate_.Hero(hero_ref).Heal(self, 2);
			return true;
		}
		Card_CS2_097() {
			RegisterEvent<WeaponInPlayZone, NonCategorized_SelfInLambdaCapture,
				state::Events::EventTypes::BeforeAttack>();
		}
	};

	struct Card_CS2_092e : public Enchantment<Card_CS2_092e, Attack<4>, MaxHP<4>> {};
	struct Card_CS2_092 : public SpellCardBase<Card_CS2_092> {
		Card_CS2_092() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_).Minion();
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardMinion(target).Enchant().Add<Card_CS2_092e>();
			});
		}
	};

	struct Card_CS2_093 : public SpellCardBase<Card_CS2_093> {
		Card_CS2_093() {
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				context.manipulate_.Board().Player(context.player_.Opposite()).minions_.ForEach([&](state::CardRef minion) {
					context.manipulate_.OnBoardMinion(minion).Damage(context.card_ref_, 2);
					return true;
				});
			});
		}
	};

	struct Card_CS2_094 : public SpellCardBase<Card_CS2_094> {
		Card_CS2_094() {
			onplay_handler.SetSpecifyTargetCallback([](FlowControl::onplay::context::GetSpecifiedTarget & context) {
				context.SetRequiredSpellTargets(context.player_);
			});
			onplay_handler.SetOnPlayCallback([](FlowControl::onplay::context::OnPlay const& context) {
				state::CardRef target = context.GetTarget();
				assert(target.IsValid());
				context.manipulate_.OnBoardCharacter(target).Damage(context.card_ref_, 3);
				context.manipulate_.Player(context.player_).DrawCard();
			});
		}
	};

	struct Card_CS2_088 : public MinionCardBase<Card_CS2_088> {
		static void Battlecry(Contexts::OnPlay const& context) {
			context.manipulate_.Hero(context.player_).Heal(context.card_ref_, 6);
		}
	};
}

REGISTER_CARD(CS2_088)
REGISTER_CARD(CS2_094)
REGISTER_CARD(CS2_093)
REGISTER_CARD(CS2_092)
REGISTER_CARD(CS2_097)
REGISTER_CARD(CS2_089)
REGISTER_CARD(EX1_360)
REGISTER_CARD(EX1_371)
REGISTER_CARD(CS2_087)
REGISTER_CARD(CS2_101)
REGISTER_CARD(CS2_101_H1)
REGISTER_CARD(CS2_101_H2)
