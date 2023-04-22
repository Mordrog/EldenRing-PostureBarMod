# EldenRing-PostureBarMod
A posture bar mode for the Elden Ring game, creates new user interface elements above the head of the enemy, which indicate how much posture damage the enemy remains.

## THIS IS EARLY VERSION - EXPECT SOME BUGS

## General
The mod was created with the intention of presenting enemy posture as a UI element inside the game, as for me it increases enjoyment of the combat system when I can have such information visible and be capable of reacting more consciously to actual enemy state rather than try to blindly assume it or spend thousands of years learning by feeling the enemy's current posture damage state. As I play-tested it, I also quickly learned the value of playing aggressively and using more posture damage techniques and weapons, which I believe ultimately made me better at playing it.

### The mod does not have ambitions to change any game data; it is only used to display it, so do not request such changes. Modularity is a nice thing; if you wish to change how the game behaves in terms of stagger values, etc., you can do that in another mod and still use it just to display data.

### The mod should be mostly compatible with other mods as it does not modify data. It may only cause issues when using a mod that heavily changes HUD elements.

![entity_posture_bar](https://user-images.githubusercontent.com/23028397/233810610-fd1235ea-e86b-4627-abb0-89b026e8c435.PNG)
![boss_posture_bar](https://user-images.githubusercontent.com/23028397/233810665-2fb3b1b3-03de-4055-9203-c1b85e00ef72.PNG)

## Instalation

First you will have to remember that you cannot play on official servers using this (or in fact any) mod.
To install mod you will have to follow this stes:
* Download Elden Mod Loader from https://www.nexusmods.com/eldenring/mods/117, and extract it's content where game .exe is
* Download this mod .zip file from {tobeadded} and extract it's contet inside "mods" folder where game .exe is, on steam path will be something like that "steamapps\common\ELDEN RING\Game\mods"
* **DISABLE ANTI CHEAT** -  you can for instance use https://www.nexusmods.com/eldenring/mods/90/%EF%BB%BF
* Run game

## Possible issues

There are some issues you might encounter after installing the mod. First, you have to remember that if the game was updated not too long ago, the last version of the mod might not have been deployed yet, and it just won't work. If for any reason the game crashes for you, especially while starting the game, you can try turning logs on inside the PostureBarModConfig.ini file and reporting the issue on the git or Nexus mod pages.

Here are some common issues you might encounter:

* ***Bars are visible while menu is open*** - currently I have no good solution for that, as right now mod just reads from any health bar in game memery and try to add posture bar where it should be. If you have any idea or information how this could be handled you can share it
* ***Boss bar is visible after dying on first boss battle in game (before cave)*** - this bar is still in game memory for some reason, you will either have to restart game or start fight with new boss, where it will get replaced and issue should not appear anymore
* ***Boss bar is not well alligned with health bar*** - it might happen especially when using other mods that changes those positions, like Reforged. You will have to set new value inside .ini file for first FirstBossScreen. You can also use special tool that can help you find proper offset inside game, by setting OffsetTest to true inside .ini file
* ***Bars in general are not alligned*** - most likely mod auto position or / and auto scaling has failed, in that case you might have to set them yourself, to figure out how those parameters should be set you can set OffsetTest to true and start game, from there you will have some options to change offsets and save them inside .ini file. In extreme case where nothing seems to work, you should report this issue on git or nexus mod page.
* ***Bars are alligned when static, but they shake a lot when moving*** - most likely it happens due to position fixing algorithm, you can turn it off inside .ini file, but then bars will always slightly drag after health bars OR you can try to adjust modifier values PositionFixingMultiplier inside .ini file or inside game by setting OffsetTest to true inside .ini file (remember to save those values). If posture bars tend to drag behind health bar then PositionFixingMultiplier should be bigger, if they tend to go ahead of health bar, then PositionFixingMultiplier should be lower
* ***Mod does not work / Mod crashes on start*** - report issue on git or nexus mod page, best to include logs, you can generate those by setting Log to true inside .ini file

## ToDo

* Fix potential new bugs (still early version)
* Add option to allow for user made bar styles (loaded as resources from graphic files)
* Fix issue with bars being shown on menus etc.

## Credits
**[Nordgaren](https://github.com/Nordgaren)** author of [ERD-Tools](https://github.com/Nordgaren/Erd-Tools)\
**[ImAxel0](https://github.com/ImAxel0)** author of [Elden-Menu](https://github.com/ImAxel0/Elden-Menu)\
**[NightFyre](https://github.com/NightFyre)** author of [ELDENRING-INTERNAL](https://github.com/NightFyre/ELDENRING-INTERNAL) that I based a lot of my source code from\
**Ingart** for being pasionate tester the best brother

## Changelog

### Beta 0.1  

* Initial version, should display simple UI bars
