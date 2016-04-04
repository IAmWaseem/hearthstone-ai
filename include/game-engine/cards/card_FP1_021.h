#ifndef GAME_ENGINE_CARDS_CARD_FP1_021
#define GAME_ENGINE_CARDS_CARD_FP1_021

DEFINE_CARD_CLASS_START(FP1_021)
// Weapon: Death's Bite
			
static void WeaponDeathrattle(Hero & equipped_hero)
{
	StageHelper::DealDamage(BoardTargets::AllMinions(equipped_hero.GetBoard()), 1, false);
}

static void Weapon_AfterEquipped(Hero & equipped_hero)
{
	equipped_hero.AddWeaponOnDeathTrigger(Hero::WeaponOnDeathTrigger(WeaponDeathrattle));
}
DEFINE_CARD_CLASS_END()
#endif
