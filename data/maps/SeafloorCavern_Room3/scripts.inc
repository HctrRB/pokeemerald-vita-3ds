SeafloorCavern_Room3_MapScripts::
	.byte 0

SeafloorCavern_Room3_EventScript_Shelly::
	trainerbattle_single TRAINER_SHELLY_SEAFLOOR_CAVERN, SeafloorCavern_Room3_Text_ShellyIntro, SeafloorCavern_Room3_Text_ShellyDefeat
	msgbox SeafloorCavern_Room3_Text_ShellyPostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room3_EventScript_Grunt5::
	trainerbattle_single TRAINER_GRUNT_SEAFLOOR_CAVERN_5, SeafloorCavern_Room3_Text_Grunt5Intro, SeafloorCavern_Room3_Text_Grunt5Defeat
	msgbox SeafloorCavern_Room3_Text_Grunt5PostBattle, MSGBOX_AUTOCLOSE
	end

SeafloorCavern_Room3_Text_ShellyIntro:
	.string "Ahahahaha!\p"
	.string "Como chegou aqui sem submarino?\n"
	.string "Que criança impressionante!\p"
	.string "Mas… Não dá pra deixar você se\n"
	.string "metendo aqui.\p"
	.string "E eu quero o troco pelo que houve\n"
	.string "no INSTITUTO DO CLIMA…\p"
	.string "Vou te dar um gostinho de dor!\n"
	.string "Conforme-se!$"

SeafloorCavern_Room3_Text_ShellyDefeat:
	.string "Ahahahaha!\p"
	.string "Ai!$"

SeafloorCavern_Room3_Text_ShellyPostBattle:
	.string "Ahahahaha!\n"
	.string "Você é forte demais.\p"
	.string "Que pena você não ser da EQUIPE\n"
	.string "AQUA.\p"
	.string "Podia curtir o mundo fabuloso que o\n"
	.string "nosso CHEFE prometeu, como um\l"
	.string "de nós…$"

SeafloorCavern_Room3_Text_Grunt5Intro:
	.string "Pro nosso sonho virar realidade,\n"
	.string "precisamos do poder dos POKéMON.\p"
	.string "Mas intrometidos como você usam\n"
	.string "o poder dos POKéMON pra atrapalhar\l"
	.string "até num lugar destes!\p"
	.string "A vida não funciona do jeito que\n"
	.string "a gente precisa!$"

SeafloorCavern_Room3_Text_Grunt5Defeat:
	.string "Guah!$"

SeafloorCavern_Room3_Text_Grunt5PostBattle:
	.string "Sabe, a gente não ousa questionar\n"
	.string "os motivos do nosso líder.\p"
	.string "Mas aqui está você, uma criança\n"
	.string "qualquer, enfrentando o nosso CHEFE.\p"
	.string "Talvez…\n"
	.string "Você seja alguém especial…$"

