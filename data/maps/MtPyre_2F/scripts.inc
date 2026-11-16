MtPyre_2F_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, CaveHole_CheckFallDownHole
	map_script MAP_SCRIPT_ON_TRANSITION, CaveHole_FixCrackedGround
	map_script MAP_SCRIPT_ON_RESUME, MtPyre_2F_SetHoleWarp
	.byte 0

MtPyre_2F_SetHoleWarp:
	setstepcallback STEP_CB_CRACKED_FLOOR
	setholewarp MAP_MT_PYRE_1F
	end

MtPyre_2F_EventScript_Woman::
	msgbox MtPyre_2F_Text_MemoriesOfSkitty, MSGBOX_NPC
	end

MtPyre_2F_EventScript_PokefanM::
	msgbox MtPyre_2F_Text_TumbledFromFloorAbove, MSGBOX_NPC
	end

MtPyre_2F_EventScript_Mark::
	trainerbattle_single TRAINER_MARK, MtPyre_2F_Text_MarkIntro, MtPyre_2F_Text_MarkDefeat
	msgbox MtPyre_2F_Text_MarkPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_2F_EventScript_Luke::
	trainerbattle_double TRAINER_DEZ_AND_LUKE, MtPyre_2F_Text_LukeIntro, MtPyre_2F_Text_LukeDefeat, MtPyre_2F_Text_LukeNotEnoughMons
	msgbox MtPyre_2F_Text_LukePostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_2F_EventScript_Dez::
	trainerbattle_double TRAINER_DEZ_AND_LUKE, MtPyre_2F_Text_DezIntro, MtPyre_2F_Text_DezDefeat, MtPyre_2F_Text_DezNotEnoughMons
	msgbox MtPyre_2F_Text_DezPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_2F_EventScript_Leah::
	trainerbattle_single TRAINER_LEAH, MtPyre_2F_Text_LeahIntro, MtPyre_2F_Text_LeahDefeat
	msgbox MtPyre_2F_Text_LeahPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_2F_EventScript_Zander::
	trainerbattle_single TRAINER_ZANDER, MtPyre_2F_Text_ZanderIntro, MtPyre_2F_Text_ZanderDefeat
	msgbox MtPyre_2F_Text_ZanderPostBattle, MSGBOX_AUTOCLOSE
	end

MtPyre_2F_Text_MemoriesOfSkitty:
	.string "Lembranças do meu querido SKITTY…\n"
	.string "Os meus olhos se enchem de lágrimas.$"

MtPyre_2F_Text_TumbledFromFloorAbove:
	.string "Ai, ui… Tem buracos no chão aqui\n"
	.string "e ali.\p"
	.string "Não percebi e caí do andar de cima.$"

MtPyre_2F_Text_MarkIntro:
	.string "Ei! Está procurando POKéMON?\n"
	.string "Você veio atrás de mim! Que falta\l"
	.string "de educação!$"

MtPyre_2F_Text_MarkDefeat:
	.string "Aiiiii!\n"
	.string "Desculpa, me perdoa, por favor!$"

MtPyre_2F_Text_MarkPostBattle:
	.string "Quase ninguém vem aqui, então\n"
	.string "achei que teria POKéMON raros.$"

MtPyre_2F_Text_LukeIntro:
	.string "LUKE: Viemos aqui por um desafio.\p"
	.string "Hehe, se eu mostrar pra ela como sou\n"
	.string "legal, ela se apaixona. Tenho certeza!\p"
	.string "Já sei! Vou acabar com você e mostrar\n"
	.string "pra ela como sou legal!$"

MtPyre_2F_Text_LukeDefeat:
	.string "LUKE: Opa!$"

MtPyre_2F_Text_LukePostBattle:
	.string "LUKE: Bom, perdemos, mas tudo bem!\n"
	.string "Estou aqui do seu lado.\l"
	.string "Vamos cumprir esse desafio!$"

MtPyre_2F_Text_LukeNotEnoughMons:
	.string "LUKE: Se quiser me enfrentar, traga\n"
	.string "mais POKéMON.\p"
	.string "Senão, não consigo mostrar pra minha\n"
	.string "namorada como sou legal!$"

MtPyre_2F_Text_DezIntro:
	.string "DEZ: Vim aqui num desafio com o meu\n"
	.string "namorado.\p"
	.string "Dá muito medo, mas estou com o meu\n"
	.string "namorado. Tudo bem.\p"
	.string "Já sei! Vou fazer o meu namorado\n"
	.string "ficar legal vencendo você!$"

MtPyre_2F_Text_DezDefeat:
	.string "DEZ: Buáááá! Que medo!$"

MtPyre_2F_Text_DezPostBattle:
	.string "DEZ: A gente é grudadinho, então\n"
	.string "não liga de perder!$"

MtPyre_2F_Text_DezNotEnoughMons:
	.string "DEZ: Se quiser desafiar a gente, traga\n"
	.string "pelo menos dois POKéMON.\p"
	.string "O meu namorado é forte.\n"
	.string "Um POKéMON só não dá nem pro cheiro.$"

MtPyre_2F_Text_LeahIntro:
	.string "Não conheço você…\n"
	.string "Vá embora antes que algo te aconteça!$"

MtPyre_2F_Text_LeahDefeat:
	.string "Hmm…\n"
	.string "Você é resistente.$"

MtPyre_2F_Text_LeahPostBattle:
	.string "A minha família treina aqui desde o\n"
	.string "tempo da minha bisavó…\p"
	.string "É meu dever proteger esta\n"
	.string "montanha…$"

MtPyre_2F_Text_ZanderIntro:
	.string "Kiaaaaah!\n"
	.string "Que pavor!$"

MtPyre_2F_Text_ZanderDefeat:
	.string "Nãããão!\n"
	.string "Perdi a cabeça!$"

MtPyre_2F_Text_ZanderPostBattle:
	.string "Eu me apavoro toda vez que vejo\n"
	.string "alguma coisa se mexer…\p"
	.string "Não devia ter vindo treinar aqui…$"

