# EldenRing-PostureBarMod
A posture bar mode for the Elden Ring game, creates new user interface elements above the head of the enemy, which indicate how much posture damage the enemy remains.

## THIS IS EARLY VERSION - EXPECT SOME BUGS

## General
The mod was created with the intention of presenting enemy posture as a UI element inside the game, as for me it increases enjoyment of the combat system when I can have such information visible and be capable of reacting more consciously to actual enemy state rather than try to blindly assume it or spend thousands of years learning by feeling the enemy's current posture damage state. As I play-tested it, I also quickly learned the value of playing aggressively and using more posture damage techniques and weapons, which I believe ultimately made me better at playing it.

### The mod does not have ambitions to change any game data; it is only used to display it, so do not request such changes. Modularity is a nice thing; if you wish to change how the game behaves in terms of stagger values, etc., you can do that in another mod and still use it just to display data.

### The mod should be mostly compatible with other mods as it does not modify data. It may only cause issues when using a mod that heavily changes HUD elements.

![posture_bar_texture](https://user-images.githubusercontent.com/23028397/236690919-6ed2b655-0eb2-41f1-aa92-8a6eb3204207.PNG)

## Instalation

First you will have to remember that you cannot play on official servers using this (or in fact any) mod.
To install mod you will have to follow this stes:
* Download Elden Mod Loader from https://www.nexusmods.com/eldenring/mods/117, and extract it's content where game .exe is
* Download this mod .zip file from [Nexus Mods](https://www.nexusmods.com/eldenring/mods/3405) or [Github](https://github.com/Mordrog/EldenRing-PostureBarMod/releases/) and extract it's contet inside "mods" folder where game .exe is, on steam path will be something like that "steamapps\common\ELDEN RING\Game\mods"
* **DISABLE ANTI CHEAT** -  you can for instance use https://www.nexusmods.com/eldenring/mods/90/%EF%BB%BF
* Run game

## Possible issues

There are some issues you might encounter after installing the mod. First, you have to remember that if the game was updated not too long ago, the last version of the mod might not have been deployed yet, and it just won't work. If for any reason the game crashes for you, especially while starting the game, you can try turning logs on inside the PostureBarModConfig.ini file and reporting the issue on the git or Nexus mod pages.

Here are some common issues you might encounter:

* ***Boss bar is not well aligned with health bar*** - it might happen especially when using other mods that changes those positions, like Reforged. You will have to set new value inside .ini file for first FirstBossScreen. You can also use special tool that can help you find proper offset inside game, by setting OffsetTest to true inside .ini file
* ***Bars in general are not aligned*** - most likely mod auto position or / and auto scaling has failed, in that case you might have to set them yourself, to figure out how those parameters should be set you can set OffsetTest to true and start game, from there you will have some options to change offsets and save them inside .ini file. In extreme case where nothing seems to work, you should report this issue on git or nexus mod page.
* ***Bars are aligned when static, but they shake a lot when moving*** - most likely it happens due to position fixing algorithm, you can turn it off inside .ini file, but then bars will always slightly drag after health bars OR you can try to adjust modifier values PositionFixingMultiplier inside .ini file or inside game by setting OffsetTest to true inside .ini file (remember to save those values). If posture bars tend to drag behind health bar then PositionFixingMultiplier should be bigger, if they tend to go ahead of health bar, then PositionFixingMultiplier should be lower
* ***Mod does not work / Mod crashes on start*** - report issue on git or nexus mod page, best to include logs, you can generate those by setting Log to true inside .ini file

## Credits
**[Nordgaren](https://github.com/Nordgaren)** author of [ERD-Tools](https://github.com/Nordgaren/Erd-Tools)\
**[ImAxel0](https://github.com/ImAxel0)** author of [Elden-Menu](https://github.com/ImAxel0/Elden-Menu)\
**[NightFyre](https://github.com/NightFyre)** author of [ELDENRING-INTERNAL](https://github.com/NightFyre/ELDENRING-INTERNAL) that I based a lot of my source code from\
**Ingart** for being passionate tester the best brother

## Changelog

### Beta 0.4.2
* Updated AOB and functionality of checking for open menus

### Beta 0.4.0
* Updated way mod is checking for open menus, also added option to disable this feature if it proofs not working for whatever reason
* Changed way mod is looking for resources etc. to assume .dll relative path
* Added stagger bar for player (not functional by default, usable only with mods that make use of player stagger values)

### Beta 0.3.0
* Added textures for UI bars
* Moved bars default offset position under HP bar to match new texture bars
* Added option to change color of bars
* Mostly fixed issues with bars appearing during menu
* Fixed issue where bar would persist until game reset or next boss fight
* Fixed issue with debug version, as it seemed to not be working at all for most people

### Beta 0.2.0
* Increased amount of logging + made special debug version with even more logging
* Made finding of Elden Ring window use Unicode format (to hopefuly fix treadmark symbol issue)
* Added option to turn of drawing of bars for entity or bosses in configs

### Beta 0.1.0

* Initial version, should display simple UI bars
