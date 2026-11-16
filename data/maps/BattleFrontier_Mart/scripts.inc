BattleFrontier_Mart_MapScripts::
	.byte 0

BattleFrontier_Mart_EventScript_Clerk::
	lock
	faceplayer
	message gText_HowMayIServeYou
	waitmessage
	pokemart BattleFrontier_Mart_Pokemart
	msgbox gText_PleaseComeAgain, MSGBOX_DEFAULT
	release
	end

	.align 2
BattleFrontier_Mart_Pokemart:
	.short ITEM_ULTRA_BALL
	.short ITEM_HYPER_POTION
	.short ITEM_MAX_POTION
	.short ITEM_FULL_RESTORE
	.short ITEM_FULL_HEAL
	.short ITEM_REVIVE
	.short ITEM_MAX_REPEL
	.short ITEM_PROTEIN
	.short ITEM_CALCIUM
	.short ITEM_IRON
	.short ITEM_ZINC
	.short ITEM_CARBOS
	.short ITEM_HP_UP
	pokemartlistend

BattleFrontier_Mart_EventScript_OldMan::
	msgbox BattleFrontier_Mart_Text_ChaperonGrandson, MSGBOX_NPC
	end

BattleFrontier_Mart_EventScript_OldWoman::
	lock
	applymovement LOCALID_FRONTIER_MART_OLD_WOMAN, Common_Movement_FaceDown
	waitmovement 0
	msgbox BattleFrontier_Mart_Text_ProteinMakeNiceGift, MSGBOX_DEFAULT
	release
	end

BattleFrontier_Mart_EventScript_Boy::
	msgbox BattleFrontier_Mart_Text_FacilitiesDontAllowItems, MSGBOX_NPC
	end

BattleFrontier_Mart_Text_ChaperonGrandson:
	.string "Viemos acompanhar o nosso neto.\p"
	.string "Mas, já que estamos aqui, pensamos\n"
	.string "em levar umas lembrancinhas.$"

BattleFrontier_Mart_Text_ProteinMakeNiceGift:
	.string "Querido, o que acha disto?\n"
	.string "Não seria um bom presente?\p"
	.string "É… PRO-TE-Í-NA?\n"
	.string "Parece gostoso, não parece?$"

BattleFrontier_Mart_Text_FacilitiesDontAllowItems:
	.string "Muitas instalações da FRONTEIRA não\n"
	.string "deixam usar itens nas batalhas.\p"
	.string "Essa regra deixa tudo ainda mais\n"
	.string "difícil!$"

