/* Mock game state for the design screens. Shape mirrors what the runtime reads from the game on
   Core 1 (gPlayerParty, bag pockets, Pokédex flags, mart inventory, Match Call, options).
   Values are plausible for a save with 4 badges; contest stats are 0..255 as in the game. */
window.GAME = {
  player: { name: 'BRENDAN', id: '24891', money: 3000, time: '12:47', badges: 4, dexSeen: 31, dexOwned: 14,
            // START menu status bar (vita_ui.c header): clock with day/night + speed, repel or Safari, registered item, run
            clock: '10:42', night: false, clockSpeed: 1, repelSteps: 87, safari: false, safariBalls: 30, runningShoes: true, autoRun: false,
            saveSlot: 1, saveOtherGame: false,
            // Trainer card back (records)
            records: [['HALL OF FAME DEBUT', '0 TIMES'], ['POKéMON CAUGHT', '14'], ['TRADES', '0'], ['POKéBLOCKS WITH FRIENDS', '3'], ['CONTESTS WON', '1'], ['BATTLE FRONTIER BP', '0']] },

  party: [
    { sp: 'mudkip', name: 'MUDKIP', ot: 'BRENDAN', otId: '24891', metAt: 'LITTLEROOT TOWN', metLv: 5, abilityDesc: 'Ups WATER moves in a pinch.', shiny: false, nextLv: 312, g: '♂', lv: 15, hp: 46, maxhp: 46, status: '', item: 'ORAN BERRY', nature: 'ADAMANT', ability: 'TORRENT',
      types: ['water'], no: 258, exp: 312, expPct: 62,
      stats: { HP: [46, 12, 31], ATTACK: [27, 20, 24], DEFENSE: [21, 4, 18], 'SP. ATK': [18, 0, 9], 'SP. DEF': [20, 0, 27], SPEED: [19, 8, 14] }, up: 'ATTACK', down: 'SP. ATK',
      moves: [['TACKLE', 'normal', 35, 35, 35, 95, 'PHYSICAL', 'A full-body charge attack.', 'contest_tough', 4, 0, 'A basic appeal.'],
              ['GROWL', 'normal', 40, 40, '-', 100, 'STATUS', 'Growls cutely to reduce the foe\'s ATTACK.', 'contest_cute', 2, 2, 'Startles the one in front.'],
              ['MUD-SLAP', 'ground', 8, 10, 20, 100, 'SPECIAL', 'Hurls mud in the foe\'s face to reduce its accuracy.', 'contest_cute', 3, 1, 'Startles those who appealed.'],
              ['WATER GUN', 'water', 21, 25, 40, 100, 'SPECIAL', 'Squirts water to attack the foe.', 'contest_cute', 4, 0, 'A basic appeal.']],
      field: ['ROCK SMASH', 'SURF'], cond: { cool: 65, beauty: 45, cute: 130, smart: 80, tough: 185, sheen: 12 }, taste: 'Likes SPICY · dislikes DRY',
      ribbons: [1, 17, 24] },
    { sp: 'taillow', name: 'TAILLOW', ot: 'BRENDAN', otId: '24891', metAt: 'ROUTE 104', metLv: 4, abilityDesc: 'Ups ATTACK if suffering.', shiny: true, nextLv: 210, g: '♀', lv: 12, hp: 27, maxhp: 32, status: '', item: '', nature: 'BRAVE', ability: 'GUTS',
      types: ['normal', 'flying'], no: 276, exp: 140, expPct: 40,
      stats: { HP: [32, 0, 20], ATTACK: [24, 8, 22], DEFENSE: [14, 0, 11], 'SP. ATK': [13, 0, 5], 'SP. DEF': [13, 0, 19], SPEED: [26, 12, 30] }, up: 'ATTACK', down: 'SPEED',
      moves: [['PECK', 'flying', 30, 35, 35, 100, 'PHYSICAL', 'Attacks the foe with a jabbing beak.', 'contest_cool', 4, 0, 'A basic appeal.'],
              ['GROWL', 'normal', 40, 40, '-', 100, 'STATUS', 'Growls cutely to reduce the foe\'s ATTACK.', 'contest_cute', 2, 2, 'Startles the one in front.'],
              ['FOCUS ENERGY', 'normal', 30, 30, '-', '-', 'STATUS', 'Focuses power to raise the critical-hit ratio.', 'contest_cool', 1, 0, 'Raises the next appeal.'],
              ['QUICK ATTACK', 'normal', 28, 30, 40, 100, 'PHYSICAL', 'An extremely fast attack that always strikes first.', 'contest_cool', 2, 0, 'The next appeal goes first.']],
      field: ['FLY'], cond: { cool: 220, beauty: 85, cute: 50, smart: 95, tough: 130, sheen: 30 }, taste: 'Likes SPICY · dislikes SWEET', ribbons: [2] },
    { sp: 'ralts', name: 'RALTS', ot: 'BRENDAN', otId: '24891', metAt: 'ROUTE 102', metLv: 4, abilityDesc: 'Passes on status problems.', shiny: false, nextLv: 160, g: '♂', lv: 10, hp: 12, maxhp: 27, status: 'PSN', item: '', nature: 'MODEST', ability: 'SYNCHRONIZE',
      types: ['psychic'], no: 280, exp: 90, expPct: 35,
      stats: { HP: [27, 0, 15], ATTACK: [9, 0, 6], DEFENSE: [10, 0, 21], 'SP. ATK': [19, 4, 28], 'SP. DEF': [16, 0, 17], SPEED: [15, 0, 12] }, up: 'SP. ATK', down: 'ATTACK',
      moves: [['GROWL', 'normal', 40, 40, '-', 100, 'STATUS', 'Growls cutely to reduce the foe\'s ATTACK.', 'contest_cute', 2, 2, 'Startles the one in front.'],
              ['CONFUSION', 'psychic', 22, 25, 50, 100, 'SPECIAL', 'A psychic attack that may cause confusion.', 'contest_smart', 2, 2, 'Startles all who appealed.'],
              ['FLASH', 'normal', 20, 20, '-', 70, 'STATUS', 'Looses a powerful blast of light that cuts accuracy.', 'contest_beauty', 2, 0, 'Excites the audience.']],
      field: ['FLASH'], cond: { cool: 40, beauty: 195, cute: 160, smart: 210, tough: 35, sheen: 60 }, taste: 'Likes BITTER · dislikes SPICY', ribbons: [] },
    { sp: 'zigzagoon', name: 'ZIGZAGOON', ot: 'BRENDAN', otId: '24891', metAt: 'ROUTE 101', metLv: 2, abilityDesc: 'May pick up items.', shiny: false, nextLv: 140, g: '♀', lv: 9, hp: 29, maxhp: 29, status: '', item: 'POTION', nature: 'JOLLY', ability: 'PICKUP',
      types: ['normal'], no: 263, exp: 60, expPct: 30,
      stats: { HP: [29, 4, 22], ATTACK: [13, 0, 10], DEFENSE: [12, 0, 14], 'SP. ATK': [9, 0, 3], 'SP. DEF': [12, 0, 25], SPEED: [17, 4, 26] }, up: 'SPEED', down: 'SP. ATK',
      moves: [['TACKLE', 'normal', 35, 35, 35, 95, 'PHYSICAL', 'A full-body charge attack.', 'contest_tough', 4, 0, 'A basic appeal.'],
              ['TAIL WHIP', 'normal', 30, 30, '-', 100, 'STATUS', 'Wags the tail to lower the foe\'s DEFENSE.', 'contest_cute', 1, 0, 'Raises the next appeal.'],
              ['HEADBUTT', 'normal', 15, 15, 70, 100, 'PHYSICAL', 'A ramming attack that may cause flinching.', 'contest_tough', 2, 2, 'Startles the one in front.'],
              ['CUT', 'normal', 30, 30, 50, 95, 'PHYSICAL', 'Cuts the foe with sharp scythes or claws.', 'contest_cool', 3, 0, 'A basic appeal.']],
      field: ['CUT'], cond: { cool: 75, beauty: 60, cute: 190, smart: 115, tough: 120, sheen: 20 }, taste: 'No preference', ribbons: [] },
    { sp: 'lotad', name: 'LOTAD', ot: 'BRENDAN', otId: '24891', metAt: 'ROUTE 102', metLv: 3, abilityDesc: 'Raises SPEED in rain.', shiny: false, nextLv: 120, g: '♂', lv: 8, hp: 4, maxhp: 26, status: '', item: '', nature: 'CALM', ability: 'SWIFT SWIM',
      types: ['water', 'grass'], no: 270, exp: 40, expPct: 20,
      stats: { HP: [26, 0, 18], ATTACK: [9, 0, 7], DEFENSE: [10, 0, 30], 'SP. ATK': [11, 0, 16], 'SP. DEF': [14, 0, 20], SPEED: [9, 0, 8] }, up: 'SP. DEF', down: 'ATTACK',
      moves: [['ASTONISH', 'ghost', 15, 15, 30, 100, 'PHYSICAL', 'An attack that may shock the foe into flinching.', 'contest_smart', 2, 3, 'Badly startles the one in front.'],
              ['GROWL', 'normal', 40, 40, '-', 100, 'STATUS', 'Growls cutely to reduce the foe\'s ATTACK.', 'contest_cute', 2, 2, 'Startles the one in front.'],
              ['ABSORB', 'grass', 25, 25, 20, 100, 'SPECIAL', 'An attack that absorbs half the damage inflicted.', 'contest_smart', 2, 0, 'Good with the same type.']],
      field: [], cond: { cool: 30, beauty: 110, cute: 140, smart: 70, tough: 60, sheen: 0 }, taste: 'Likes BITTER · dislikes SPICY', ribbons: [] },
    // An egg: no level, HP, moves or stats; it cannot battle, hold items, learn moves or eat Pokéblocks
    { sp: 'egg', egg: true, name: 'EGG', g: '', lv: 0, hp: 0, maxhp: 0, status: '', item: '', nature: '', ability: '', types: [], no: 0, exp: 0, expPct: 0,
      stats: {}, moves: [], field: [], cond: { cool: 0, beauty: 0, cute: 0, smart: 0, tough: 0, sheen: 0 }, taste: '', ribbons: [],
      eggNote: 'Sounds can be heard coming from inside! It will hatch soon!' },
  ],

  // Bag: the 5 Emerald pockets. price = buy price (sell = half). key items cannot be sold.
  bag: {
    items: [['potion', 'POTION', 3, 300, 'Restores the HP of a POKéMON by 20 points.'], ['super_potion', 'SUPER POTION', 2, 700, 'Restores the HP of a POKéMON by 50 points.'],
            ['antidote', 'ANTIDOTE', 1, 100, 'Heals a poisoned POKéMON.'], ['awakening', 'AWAKENING', 2, 250, 'Awakens a sleeping POKéMON.'],
            ['full_heal', 'FULL HEAL', 1, 600, 'Heals all the status problems of one POKéMON.'], ['escape_rope', 'ESCAPE ROPE', 1, 550, 'Use to escape instantly from a cave or a dungeon.'],
            ['repel', 'REPEL', 2, 350, 'Repels weak wild POKéMON for 100 steps.'], ['revive', 'REVIVE', 1, 1500, 'Revives a fainted POKéMON with half its HP.'],
            ['ether', 'ETHER', 1, 1200, 'Restores a selected move by 10 PP.'], ['rare_candy', 'RARE CANDY', 1, 4800, 'Raises the level of a POKéMON by one.'],
            ['nugget', 'NUGGET', 1, 10000, 'A nugget of pure gold that gives off a lustrous gleam.'], ['soothe_bell', 'SOOTHE BELL', 1, 100, 'A hold item that calms spirits and fosters friendship.']],
    balls: [['poke_ball', 'POKé BALL', 14, 200, 'A tool used for catching wild POKéMON.'], ['great_ball', 'GREAT BALL', 6, 600, 'A good BALL with a higher catch rate than a POKé BALL.'],
            ['ultra_ball', 'ULTRA BALL', 3, 1200, 'A better BALL with a higher catch rate than a GREAT BALL.'], ['net_ball', 'NET BALL', 4, 1000, 'A BALL that works well on WATER- and BUG-type POKéMON.'],
            ['dive_ball', 'DIVE BALL', 2, 1000, 'A BALL that works better on POKéMON on the ocean floor.'], ['timer_ball', 'TIMER BALL', 2, 1000, 'More effective as more turns are taken in battle.'],
            ['repeat_ball', 'REPEAT BALL', 5, 1000, 'Works well on POKéMON species that were previously caught.'], ['nest_ball', 'NEST BALL', 3, 1000, 'Works better on weaker POKéMON.'],
            ['luxury_ball', 'LUXURY BALL', 1, 1000, 'A cozy BALL that makes POKéMON more friendly.'], ['premier_ball', 'PREMIER BALL', 1, 200, 'A rare BALL made in commemoration of some event.'],
            ['master_ball', 'MASTER BALL', 1, 0, 'The best BALL. It catches a wild POKéMON without fail.']],
    tms: [['tm28', 'TM28 DIG', 1, 2000, 'Digs underground the first turn and strikes the next turn.'], ['tm39', 'TM39 ROCK TOMB', 1, 2000, 'Stops the foe\'s movement with rocks. May lower SPEED.'],
          ['tm01', 'TM01 FOCUS PUNCH', 1, 3000, 'Powerful, but makes the user flinch if hit first.'], ['hm01', 'HM01 CUT', 1, 0, 'Cuts the foe with sharp scythes or claws. Field use: cut trees.']],
    berries: [['oran_berry', 'ORAN BERRY', 5, 20, 'A hold item that restores 10 HP in battle.'], ['pecha_berry', 'PECHA BERRY', 2, 20, 'A hold item that heals poisoning in battle.']],
    key: [['mach_bike', 'MACH BIKE', 1, 0, 'A folding bicycle that doubles your speed or better.'], ['old_rod', 'OLD ROD', 1, 0, 'Use by any body of water to fish for wild POKéMON.'],
          ['pokeblock_case', 'POKéBLOCK CASE', 1, 0, 'A case for holding POKéBLOCKS made with a BERRY BLENDER.']],
  },
  pockets: [['items', 'ITEMS', 1], ['balls', 'POKé BALLS', 2], ['tms', 'TMs & HMs', 3], ['berries', 'BERRIES', 4], ['key', 'KEY ITEMS', 5]],
  registered: 'mach_bike',

  mart: { name: 'PETALBURG CITY', stock: [['poke_ball', 'balls'], ['potion', 'items'], ['antidote', 'items'], ['paralyze_heal', null, 'PARALYZE HEAL', 200, 'Heals a paralyzed POKéMON.'],
                                          ['awakening', 'items'], ['escape_rope', 'items'], ['repel', 'items'], ['burn_heal', null, 'BURN HEAL', 250, 'Heals a POKéMON of a burn.'],
                                          ['ice_heal', null, 'ICE HEAL', 250, 'Defrosts a frozen POKéMON.'], ['x_speed', null, 'X SPEED', 350, 'Raises SPEED during one battle.']] },

  // Hoenn Pokédex (first entries). st: 0 unknown, 1 seen, 2 caught. nat = National number.
  dex: [['treecko', 'TREECKO', 252, 2, 'WOOD GECKO', ['grass'], "1'08\"", '11.0 lbs', 'TREECKO has small hooks on the bottom of its feet that enable it to scale vertical walls.'],
        ['grovyle', 'GROVYLE', 253, 2, 'WOOD GECKO', ['grass'], "2'11\"", '47.6 lbs', 'The leaves growing out of GROVYLE\'s body are convenient for camouflaging it in forests.'],
        ['sceptile', 'SCEPTILE', 254, 1, 'FOREST', ['grass']], ['torchic', 'TORCHIC', 255, 1, 'CHICK', ['fire']], ['combusken', 'COMBUSKEN', 256, 1, 'YOUNG FOWL', ['fire', 'fight']],
        ['blaziken', 'BLAZIKEN', 257, 0], ['mudkip', 'MUDKIP', 258, 2, 'MUD FISH', ['water'], "1'04\"", '16.8 lbs', 'The fin on MUDKIP\'s head acts as highly sensitive radar.'],
        ['marshtomp', 'MARSHTOMP', 259, 1, 'MUD FISH', ['water', 'ground']], ['swampert', 'SWAMPERT', 260, 0],
        ['poochyena', 'POOCHYENA', 261, 2, 'BITE', ['dark'], "1'08\"", '30.0 lbs', 'POOCHYENA is an omnivore - it will eat anything.'],
        ['mightyena', 'MIGHTYENA', 262, 1, 'BITE', ['dark']], ['zigzagoon', 'ZIGZAGOON', 263, 2, 'TINYRACCOON', ['normal'], "1'04\"", '38.6 lbs', 'The hair on ZIGZAGOON\'s back is bristly.'],
        ['linoone', 'LINOONE', 264, 1, 'RUSHING', ['normal']], ['wurmple', 'WURMPLE', 265, 2, 'WORM', ['bug'], "1'00\"", '7.9 lbs', 'Using the spikes on its rear end, WURMPLE peels the bark off trees.'],
        ['silcoon', 'SILCOON', 266, 2, 'COCOON', ['bug'], "2'00\"", '22.0 lbs', 'SILCOON tethers itself to a tree branch using silk.'], ['beautifly', 'BEAUTIFLY', 267, 1, 'BUTTERFLY', ['bug', 'flying']],
        ['cascoon', 'CASCOON', 268, 0], ['dustox', 'DUSTOX', 269, 0], ['lotad', 'LOTAD', 270, 2, 'WATER WEED', ['water', 'grass'], "1'08\"", '5.7 lbs', 'LOTAD live in ponds and lakes, where they float on the surface.'],
        ['lombre', 'LOMBRE', 271, 1, 'JOLLY', ['water', 'grass']], ['ludicolo', 'LUDICOLO', 272, 0], ['seedot', 'SEEDOT', 273, 1, 'ACORN', ['grass']], ['nuzleaf', 'NUZLEAF', 274, 0], ['shiftry', 'SHIFTRY', 275, 0]],

  contacts: [['MOM', 'LITTLEROOT', '', 'AT HOME', ['Hi, honey! Are you eating well?', 'Your father would be proud of you!']],
             ['MAY', 'ROUTE 104', 'may', 'TRAINING', ['Hi, BRENDAN! How\'s your POKéDEX coming along?', 'I just caught a ZIGZAGOON!']],
             ['MR. STONE', 'RUSTBORO', '', 'DEVON CORP.', ['Hello, BRENDAN! Thanks for delivering the letter to my son.']],
             ['ROSE (AROMA LADY)', 'ROUTE 118', '', 'REMATCH', ['The flowers smell wonderful today. Shall we battle again?']],
             ['ANDRES (RUIN MANIAC)', 'ROUTE 105', '', 'EXPLORING', ['I\'m searching for ancient ruins on ROUTE 105.']],
             ['ROXANNE (LEADER)', 'RUSTBORO', 'roxanne', 'GYM', ['Have you been studying? Knowledge is power in battle.']],
             ['BRAWLY (LEADER)', 'DEWFORD', 'brawly', 'GYM', ['The waves are great today! Come surf the big wave of battle!']],
             ['WATTSON (LEADER)', 'MAUVILLE', 'wattson', 'GYM', ['Wahahahah! Charged up and ready!']],
             ['FLANNERY (LEADER)', 'LAVARIDGE', 'flannery', 'GYM', ['I\'m training hard to be worthy of my grandfather!']],
             ['ISABEL (POKéFAN)', 'ROUTE 110', '', 'REMATCH', ['My POKéMON want to see yours again!']],
             ['KALEB (POKéFAN)', 'ROUTE 110', '', 'RESTING', ['My wife and I love our POKéMON.']],
             ['JOSE (BUG CATCHER)', 'ROUTE 104', '', 'CATCHING', ['I found a rare BUG POKéMON in PETALBURG WOODS!']]],

  // Pokéblocks in the case: colour, level, [cool, beauty, cute, smart, tough], feel
  pokeblocks: [['RED', 3, [3, 0, 0, 0, 0], 21, 0], ['BLUE', 2, [0, 2, 0, 0, 0], 22, 200], ['PINK', 4, [0, 0, 4, 0, 0], 20, 300], ['GREEN', 2, [0, 0, 0, 2, 0], 23, 90], ['YELLOW', 3, [0, 0, 0, 0, 3], 21, 50]],

  // The 32 ribbons by game id (constants/pokemon.h); sprites in _assets/ribbons/ribbon_<id>.png
  ribbons: [['CHAMPION RIBBON', 'Champion-beating, Hall of Fame Member Ribbon.'],
            ['COOL RIBBON', 'HOENN COOL CONTEST Normal Rank winner!'],
            ['COOL RIBBON SUPER', 'HOENN COOL CONTEST Super Rank winner!'],
            ['COOL RIBBON HYPER', 'HOENN COOL CONTEST Hyper Rank winner!'],
            ['COOL RIBBON MASTER', 'HOENN COOL CONTEST Master Rank winner!'],
            ['BEAUTY RIBBON', 'HOENN BEAUTY CONTEST Normal Rank winner!'],
            ['BEAUTY RIBBON SUPER', 'HOENN BEAUTY CONTEST Super Rank winner!'],
            ['BEAUTY RIBBON HYPER', 'HOENN BEAUTY CONTEST Hyper Rank winner!'],
            ['BEAUTY RIBBON MASTER', 'HOENN BEAUTY CONTEST Master Rank winner!'],
            ['CUTE RIBBON', 'HOENN CUTE CONTEST Normal Rank winner!'],
            ['CUTE RIBBON SUPER', 'HOENN CUTE CONTEST Super Rank winner!'],
            ['CUTE RIBBON HYPER', 'HOENN CUTE CONTEST Hyper Rank winner!'],
            ['CUTE RIBBON MASTER', 'HOENN CUTE CONTEST Master Rank winner!'],
            ['SMART RIBBON', 'HOENN SMART CONTEST Normal Rank winner!'],
            ['SMART RIBBON SUPER', 'HOENN SMART CONTEST Super Rank winner!'],
            ['SMART RIBBON HYPER', 'HOENN SMART CONTEST Hyper Rank winner!'],
            ['SMART RIBBON MASTER', 'HOENN SMART CONTEST Master Rank winner!'],
            ['TOUGH RIBBON', 'HOENN TOUGH CONTEST Normal Rank winner!'],
            ['TOUGH RIBBON SUPER', 'HOENN TOUGH CONTEST Super Rank winner!'],
            ['TOUGH RIBBON HYPER', 'HOENN TOUGH CONTEST Hyper Rank winner!'],
            ['TOUGH RIBBON MASTER', 'HOENN TOUGH CONTEST Master Rank winner!'],
            ['WINNING RIBBON', 'Ribbon for clearing LV50 at the BATTLE TOWER.'],
            ['VICTORY RIBBON', 'Won for clearing LV100 at the BATTLE TOWER.'],
            ['ARTIST RIBBON', 'Ribbon for being chosen as a super sketch model.'],
            ['EFFORT RIBBON', 'Ribbon awarded for being an exceptionally hard worker.'],
            ['MARINE RIBBON', 'A special gift ribbon.'],
            ['LAND RIBBON', 'A special gift ribbon.'],
            ['SKY RIBBON', 'A special gift ribbon.'],
            ['COUNTRY RIBBON', 'PokéMON LEAGUE Champion Ribbon.'],
            ['NATIONAL RIBBON', 'Overcoming difficult challenges.'],
            ['EARTH RIBBON', 'Won for clearing the MT. BATTLE challenge.'],
            ['WORLD RIBBON', 'POKéMON WORLD Champion Ribbon.']],

  badges: [['STONE', 'RUSTBORO GYM', 'ROXANNE', 'Rock'], ['KNUCKLE', 'DEWFORD GYM', 'BRAWLY', 'Fighting'], ['DYNAMO', 'MAUVILLE GYM', 'WATTSON', 'Electric'],
           ['HEAT', 'LAVARIDGE GYM', 'FLANNERY', 'Fire'], ['BALANCE', 'PETALBURG GYM', 'NORMAN', 'Normal'], ['FEATHER', 'FORTREE GYM', 'WINONA', 'Flying'],
           ['MIND', 'MOSSDEEP GYM', 'TATE & LIZA', 'Psychic'], ['RAIN', 'SOOTOPOLIS GYM', 'JUAN', 'Water']],

  // Options: the real ones (vita_ui_options.c + gVitaExtras in vita_extras.c). [group, label, kind, values, value, help]
  options: {
    general: [['GAMEPLAY', 'TEXT SPEED', 'enum', ['SLOW', 'MID', 'FAST'], 2, 'How fast messages are printed.'],
              ['GAMEPLAY', 'BATTLE SCENE', 'enum', ['ON', 'OFF'], 0, 'Battle animations of moves.'],
              ['GAMEPLAY', 'BATTLE STYLE', 'enum', ['SHIFT', 'SET'], 0, 'SHIFT: switch after a foe faints.'],
              ['AUDIO', 'SOUND OUTPUT', 'enum', ['MONO', 'STEREO'], 1, 'Audio output mode.'],
              ['AUDIO', 'MASTER VOLUME', 'slider', [0, 100, 10], 80, 'Volume of music and sounds.'],
              ['INTERFACE', 'BUTTON MODE', 'enum', ['NORMAL', 'LR', 'L=A'], 0, 'What L and R do in the game.'],
              ['CONTROLS', 'BUTTON REMAPPING', 'action', 'remapeo', null, 'Choose the Vita button of each action.'],
              ['PACING', 'FAST FWD SPEED', 'enum', ['2X', '3X', '4X', '5X'], 1, 'Speed of fast forward.'],
              ['PACING', 'FF BUTTON MODE', 'enum', ['HOLD', 'TOGGLE'], 0, 'Hold or toggle fast forward.'],
              ['DISPLAY', '2D SCALER', 'enum', ['NEAREST', 'BILINEAR', 'SHARP'], 0, 'Filter of the 2D GBA screens.']],
    world: [['RENDERER', '3D VOXEL DIORAMA', 'enum', ['OFF', 'ON'], 1, 'Voxel 3D overworld.'],
            ['RENDERER', 'DIORAMA BLUR (DOF)', 'enum', ['OFF', 'ON'], 1, 'Tilt-shift blur outdoors.'],
            ['RENDERER', 'VOXEL RESOLUTION', 'enum', ['960x544', '720x408'], 0, 'Render size of the 3D world.'],
            ['RENDERER', 'TARGET FRAMERATE', 'enum', ['30 FPS', 'LIBRE 60'], 0, 'Frame rate of the 3D world.'],
            ['TIME & SKY', 'DAY / NIGHT CYCLE', 'enum', ['OFF', 'AUTO'], 1, 'Day and night light.'],
            ['TIME & SKY', 'TIMED ENCOUNTERS', 'enum', ['OFF', 'ON'], 1, 'Wild POKéMON change with the time.'],
            ['TIME & SKY', 'CLOCK SPEED', 'enum', ['1X', '6X', '12X', '24X', '60X', '120X', '1000X'], 0, 'Speed of the game clock.'],
            ['COMPANION', 'HGSS FOLLOWER', 'enum', ['OFF', 'ON'], 1, 'The lead POKéMON follows you.'],
            ['EXPLORATION', 'RUN INDOORS', 'enum', ['OFF', 'ON'], 1, 'Allow running in buildings.'],
            ['EXPLORATION', 'AUTO RUN (NO B)', 'enum', ['OFF', 'ON'], 0, 'Run without holding B.'],
            ['EXPLORATION', 'BAG HM SHORTCUTS', 'enum', ['OFF', 'ON'], 1, 'Use field moves from the bag.'],
            ['EXPLORATION', 'REPEL PROMPT', 'enum', ['OFF', 'ON'], 1, 'Offer another REPEL when one ends.']],
    battle: [['ARENA', '3D VOXEL BATTLE', 'enum', ['OFF', 'ON'], 1, 'Battles in the 3D world.'],
             ['ARENA', 'BATTLE SPEED', 'enum', ['1X', '1.5X', '2X', '3X', '4X'], 0, 'Speed of battle animations.'],
             ['MECHANICS', 'PHYS/SPEC SPLIT', 'enum', ['OFF', 'ON'], 1, 'Moves use their own category.'],
             ['MECHANICS', 'PARTY EXP SHARE', 'enum', ['OFF', 'ON'], 0, 'The whole party gets EXP.'],
             ['MECHANICS', 'EXP ON CATCH', 'enum', ['OFF', 'ON'], 1, 'Gain EXP when catching.'],
             ['INSPECTION', 'IVS & EVS STATS', 'enum', ['OFF', 'ON'], 1, 'Show IVs and EVs in the summary.']],
    modes: [['CHALLENGE', 'NUZLOCKE MODE', 'enum', ['OFF', 'ON'], 1, 'Nuzlocke rules and tracker.'],
            ['CHALLENGE', 'LEVEL CAP', 'enum', ['OFF', 'GYM CAP', 'STRICT'], 1, 'Cap levels at the next GYM.'],
            ['PROFILES', 'ACTIVE SAVE SLOT', 'enum', ['SLOT 1', 'SLOT 2', 'SLOT 3'], 0, 'Which save file is used.'],
            ['PREVIEW', 'NEW UI (TEST)', 'enum', ['OFF', 'ON'], 1, 'New UI test screen on START.'],
            ['SYSTEM', 'QUIT TO LIVEAREA', 'action', 'salir', null, 'Close the game.']],
    cheats: [['CONVENIENCE', 'INFINITE TMS', 'enum', ['OFF', 'ON'], 1, 'TMs are not used up.'],
             ['CONVENIENCE', 'TRADE EVO LV40', 'enum', ['OFF', 'ON'], 1, 'Trade evolutions at level 40.'],
             ['CONVENIENCE', 'FAST HATCH EGGS', 'enum', ['OFF', 'ON'], 0, 'Eggs hatch in fewer steps.'],
             ['ENCOUNTERS', 'SHINY ODDS', 'enum', ['1/8192', '1/4096', '1/1024', '1/256', '1/64', 'ALWAYS'], 0, 'Chance of a shiny.'],
             ['ENCOUNTERS', 'ALWAYS CATCH', 'enum', ['OFF', 'ON'], 0, 'Every ball catches.'],
             ['REWARDS', 'MAX MONEY ($999K)', 'action', 'confirmar', null, 'Set the money to the maximum.'],
             ['ITEM SPAWN', 'GIVE POCKET', 'enum', ['ITEMS', 'BALLS', 'TMs', 'BERRIES', 'KEY'], 0, 'Pocket of the item to give.'],
             ['ITEM SPAWN', 'GIVE ITEM', 'enum', ['POTION', 'RARE CANDY', 'MASTER BALL'], 0, 'Item to give.'],
             ['ITEM SPAWN', 'GIVE COUNT', 'enum', ['1', '5', '10', '50', '99'], 0, 'How many.'],
             ['ITEM SPAWN', 'DELIVER ITEM', 'action', 'confirmar', null, 'Put the items in the bag.'],
             ['POKéDEX', 'COMPLETE HOENN DEX', 'action', 'confirmar', null, 'Mark the HOENN DEX as caught.'],
             ['POKéDEX', 'UNLOCK NATIONAL', 'action', 'confirmar', null, 'Unlock the NATIONAL DEX.'],
             ['CONTESTS', 'MAX CONTEST STATS', 'action', 'confirmar', null, 'Max the condition of the party.']],
  },
  // Button remapping: the 8 actions of vita_ui_options.c with their default buttons
  remap: [['A BUTTON', 'cross'], ['B BUTTON', 'circle'], ['L SHOULDER', 'L'], ['R SHOULDER', 'R'], ['START', 'START'], ['SELECT', 'SELECT'],
          ['FAST FORWARD', 'triangle'], ['MENU (OPTIONS)', 'square']],
  remapCycle: ['cross', 'circle', 'triangle', 'square', 'L', 'R', 'START', 'SELECT'],

  // Nuzlocke tracker (vita_ui.c / vita_nuzlocke.c)
  nuzlocke: { nextLeader: 'NORMAN', cap: 31, fallen: ['LOTAD'] },

  // START menu entries: screen + description shown while selected
  startMenu: [['POKéDEX', 'pokedex', 'POKéMON seen and caught.'],
              ['POKéMON', 'equipo', 'Check and organize your party.'],
              ['BAG', 'mochila', 'Items you bought or found.'],
              ['POKéNAV', 'pokenav_mapa', 'Map, calls, condition, ribbons.'],
              ['BRENDAN', 'ficha', 'Your money and game data.'],
              ['SAVE', '#guardar', 'Save your progress.'],
              ['OPTION', 'opciones', 'Game settings.'],
              ['NUZLOCKE', 'nuzlocke', 'Areas, fallen allies, level cap.'],
              ['EXIT', '', 'Close this menu.']],

  // Encounters per zone and method (vita_encounters.c): GRASS by time of day (TIMED ENCOUNTERS), FISH by rod
  encounters: {
    MAPSEC_ROUTE_101: { GRASS: { MORNING: [['zigzagoon', 2, 3], ['wurmple', 2, 3]], DAY: [['zigzagoon', 2, 3], ['wurmple', 2, 3], ['poochyena', 2, 3]], NIGHT: [['poochyena', 2, 3], ['zigzagoon', 2, 3]] } },
    MAPSEC_ROUTE_102: { GRASS: { MORNING: [['wurmple', 3, 4], ['lotad', 3, 4], ['seedot', 3, 4]], DAY: [['poochyena', 3, 4], ['wurmple', 3, 4], ['lotad', 3, 4], ['zigzagoon', 3, 4], ['ralts', 4, 4], ['seedot', 3, 4]], NIGHT: [['poochyena', 3, 4], ['zigzagoon', 3, 4], ['surskit', 3, 4]] },
                        SURF: [['marill', 20, 30], ['goldeen', 20, 30]],
                        FISH: { OLD: [['magikarp', 5, 10]], GOOD: [['magikarp', 10, 30], ['goldeen', 10, 30]], SUPER: [['corphish', 20, 40]] } },
    MAPSEC_ROUTE_103: { GRASS: { DAY: [['poochyena', 2, 4], ['zigzagoon', 2, 4], ['wingull', 3, 4]], NIGHT: [['poochyena', 2, 4], ['zubat', 3, 4]] },
                        SURF: [['wingull', 5, 20], ['tentacool', 5, 20]],
                        FISH: { OLD: [['magikarp', 5, 10], ['tentacool', 5, 10]], GOOD: [['magikarp', 10, 30]], SUPER: [['tentacool', 25, 30]] } },
    MAPSEC_ROUTE_104: { GRASS: { DAY: [['zigzagoon', 4, 5], ['wurmple', 4, 5], ['taillow', 4, 5], ['wingull', 4, 5]], NIGHT: [['zigzagoon', 4, 5], ['wingull', 4, 5]] },
                        SURF: [['wingull', 10, 30]], FISH: { OLD: [['magikarp', 5, 10]], GOOD: [['magikarp', 10, 30]], SUPER: [['magikarp', 25, 30]] } },
    MAPSEC_GRANITE_CAVE: { GRASS: { DAY: [['zubat', 6, 8], ['geodude', 6, 8]] }, ROCK: [['geodude', 10, 15], ['nosepass', 10, 15]] },
    MAPSEC_LITTLEROOT_TOWN: { SURF: [['tentacool', 5, 30]] },
  },
};

// Badge needed to use each field move outside battle (Emerald order)
window.GAME.hmBadge = { CUT: 1, FLASH: 2, 'ROCK SMASH': 3, STRENGTH: 4, SURF: 5, FLY: 6, DIVE: 7, WATERFALL: 8 };

/* Small helpers shared by the screens */
window.UI = {
  money: v => '₽ ' + v.toLocaleString('en-US'),
  icon: sp => `<div class="mon" style="background-image:url(../_assets/pokemon/${sp}_icon.png)"></div>`,
  item: id => `<img class="item-ico" src="../_assets/items/${id}.png" alt="" onerror="this.style.visibility='hidden'">`,
  type: t => `<img src="../_assets/types/${t}.png" alt="${t}" style="zoom:2; display:inline-block">`,
  hpClass: (hp, max) => hp / max > 0.5 ? '' : hp / max > 0.2 ? 'mid' : 'low',
  // Render rows into a list; the first gets .on; returns the row elements
  rows(list, items, html, attrs) {
    list.querySelectorAll(':scope > .row, :scope > .card').forEach(r => r.remove());
    const bar = list.querySelector('.scrollbar');
    items.forEach((it, i) => {
      const t = document.createElement('template');
      t.innerHTML = html(it, i).trim();
      const el = t.content.firstElementChild;
      el.dataset.idx = i;
      if (attrs) for (const [k, v] of Object.entries(attrs(it, i) || {})) if (v != null) el.setAttribute(k, v);
      list.insertBefore(el, bar || null);
    });
    const first = list.querySelector(':scope > .row, :scope > .card');
    if (first) first.classList.add('on');
    list.scrollTop = 0;
    if (window.rescanLists) window.rescanLists();
    return [...list.children].filter(c => c.dataset.idx != null);
  },
  onSelect(container, fn) {
    document.addEventListener('ui-select', e => { if (container.contains(e.detail)) fn(+e.detail.dataset.idx, e.detail); });
  },
  selected(container) { const el = container.querySelector(':scope > .on'); return el ? +el.dataset.idx : 0; },
  toast(text, ms = 1400) {
    let t = document.getElementById('ui-toast');
    if (!t) { t = document.createElement('div'); t.id = 'ui-toast'; t.className = 'toast'; document.querySelector('.stage').appendChild(t); }
    Object.assign(t.style, { left: '50%', top: '460px', transform: 'translateX(-50%)', zIndex: 40, display: 'flex' });
    t.textContent = text;
    clearTimeout(t._h); t._h = setTimeout(() => { t.style.display = 'none'; }, ms);
  },
};
