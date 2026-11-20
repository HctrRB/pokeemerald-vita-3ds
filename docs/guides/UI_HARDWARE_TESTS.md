# Testing the new UI on a Vita

Type: guide · Audience: testers · Status: current
Verified against: `5a679fd9`

The screens of the new UI that replaced GBA menus, and the GBA scenes that keep their own format,
need checking on a real Vita: the emulator hides timing and memory problems. This list says what to
check for each one, what was already checked on hardware and what was not. Every screen is checked in
the GBA theme and in at least one other theme (OPTIONS > GENERAL > DISPLAY > MENU COLOR).

If the game crashes, keep the `psp2core-*.psp2dmp` from `ux0:data/` and the build it came from
([DEBUGGING.md](DEBUGGING.md), "Crash dumps").

Status: **checked** (works on hardware), **to check** (built, never run on hardware), **to check
again** (fixed after a failure on hardware).

## Safari Zone

| Check | Status |
|---|---|
| In battle the player's box shows SAFARI BALLS and the balls left, in the six themes | checked |
| RETIRE in the START menu asks once | checked |
| The START menu shows a Safari Ball and "balls · steps" | checked |

## Pokéblock case

| Check | Status |
|---|---|
| From the bag and with SELECT: USE, TOSS, moving blocks, with the case empty and full | checked |
| In a Safari battle: throw a block and cancel | checked |
| At a Safari feeder and with the Contest Lady: the script goes on afterwards | checked |
| FEED plays the GBA eating scene and the result shows on return | checked |
| FEED from the PokéNav CONDITION tab plays the scene in a window over the theme's frame, then the PokéNav comes back with a message | to check |
| After feeding (and after CHECK TAG) every menu that was open comes back as it was | to check |

## GBA scenes in their own format

| Check | Status |
|---|---|
| The contest shows nothing of the new UI over it | checked |
| Berry Blender, slots, roulette, contest, Pokéblock eating, Battle Dome and Battle Factory show at 816×544 with black bars | to check |
| No flicker of the format when those scenes start or end | to check |
| The GBA window frame in those scenes follows the menu theme | to check |
| A soft reset during one of those scenes leaves nothing behind | to check |
| The Game Corner coins box and the field's scrolling lists (Game Corner prizes, Lilycove Dept. Store and its elevator, Battle Frontier exchange and tutors) are the new UI's, and their rows answer touch | to check |

## Battle

| Check | Status |
|---|---|
| Catching a new species shows the new Pokédex page over the darkened battle with its cry; A or B closes it and the nickname question follows | checked |

## Scripts that open a bag or a party

| Check | Status |
|---|---|
| Planting a berry: only BERRIES, choosing is enough | checked |
| CONTEST entry and MOVE RELEARNER choices | checked |
| The Favor Lady (GIVE takes one), the Quiz Lady (CONFIRM), the Apprentice (SHOW); important items refused | to check |
| Move Deleter: no new-move row | to check |
| Move Relearner: teach, delete an older move through FORGET and back, give up | to check |
| Move Relearner: the result can't be closed until the level-up fanfare ends (OK shows then) | to check |
| CHECK TAG from the bag, back to the BERRIES pocket | to check |
| The bag's actions on berries (CHECK TAG, USE, GIVE, TOSS) | to check |

## Single screens

| Check | Status |
|---|---|
| PC SUMMARY from a box and from the party: ↑ / ↓ through them (empty slots skipped), move order kept in the box, back on the last one looked at | checked |
| Reading a mail from the bag, from a party POKéMON and from the PC mailbox | to check again |
| Writing a mail: 9 words, the word lists not covered | to check |
| The Bard in Mauville: no word deleted, YES keeps the new song, NO brings the old one back | to check |
| The Quiz Lady in Lilycove: answering (the question can only be read, QUIZ shows it again) and making a quiz (QUIZ / ANSWER switch, quitting keeps it) | to check |
| A wall map: only the Hoenn map, ○ closes it and the script goes on | to check |
| Mr. Stone's PokéNav tutorial in Rustboro (needs a save from before it): MATCH CALL only, ○ refused until the call | to check |
| Trainer card with the Frontier Pass: MAP (map, blinking marker, descriptions) and RECORD (with and without a recorded battle; the battle plays and the card comes back on RECORD with the right music) | to check |

## Battle Pyramid and Battle Frontier

| Check | Status |
|---|---|
| START on a Pyramid floor: POKéMON, BAG, the trainer card, REST, RETIRE, OPTION; the floor shows in the status | to check |
| REST saves, then the game restarts as in the GBA (the save resumes in the Pyramid) | to check |
| RETIRE asks with NO first; YES ends the challenge through the game's script | to check |
| The PYRAMID BAG in the field (USE, GIVE, TOSS) and in battle (only what can be used) | to check |
| Giving and taking items in the party inside the Pyramid uses the PYRAMID BAG | to check |
| Leaving with a full PYRAMID BAG: "toss from the bag" opens it with TOSS only; "take from the party" lists the held items with TAKE ITEM / TOSS; the lobby's choices come back after each | to check |
| The half-party choice of the Frontier facilities and the Battle Tents: enter and take out in order (FIRST, SECOND...), NOT ABLE refused, CONFIRM with the game's checks (same species, same held item, how many), ○ asks "Cancel the challenge?" | to check |

## Every GBA scene once (the hardware pass)

Each mini-game and full-screen GBA scene once: black bars, the frame of the theme, nothing of the new
UI inside, the fades in and out. This includes the scenes not listed above: Pokémon Jump and Dodrio
Berry Picking (link), the Battle Arena judgment, the Hall of Fame, the berry tag and the naming
screen's GBA parts.

## Known problems

- Link and recorded battles, Wally's catching tutorial and the Battle Arena judgment keep the GBA
  battle HUD.
