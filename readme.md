# Window Wrangler
An application to list all open windows on windows.

to get started, `wndwrangle /?` should give you all the information you need.



### More Info:
you can use both / and - for command line arguments,
for example:
```
wndwrangle -h
wndwrangle /h
```
will both do the same thing (list hidden windows)


Commands:
```
/about
  displays version information. Exits without doing anything else.
/license
  displays the MIT License text that this program and its source is covered by. Exits without doing anything else.
/? or /help
  displays a list of commands. Exits without doing anything else.
/h or /hidden
  Lists hidden windows along with visible windows.
/v or /verbose
  Lists windows that the process thereof couldn't be determined.
/p or /displaypid
  displays the Program ID associated with the window.
/i or /insensitive
  Removes case-sensitivity from Window Title Filtering.
/c or /close
  Sends a WM_CLOSE message to listed windows (Similar to clicking on its close button.
/r or /reposition
  Repositions listed windows to the mouse (possibly you have lost it off screen?)
/f or /find [Title]
  Filters windows based on the [Title] parameter matching the window title, It will match even if the [Title] parameter is only a substring of the full window title.
/nf or /negativefind [Title]
  Filters windows based on the [Title] parameter, as with /find. however, it excludes windows that match, unlike /find, which includes said windows.
/m or /match [PID]
  Filters windows based on associated Program ID matching the [PID] parameter. matching windows are listed.
/nm or /negativematch [PID]
  Filters windows based on associated Program ID matching the [PID] parameter. However, matching windows are excluded.
/col or /color
  Enables the use of colors, defined in the registry at (HKEY_CURRENT_USER\SOFTWARE\WindowWrangler) or text file (Config.txt)
/pm or /postmessage [msg] [wparam] [lparam]
  Posts a message to the listed windows' message queues
/pm or /sendmessage [msg] [wparam] [lparam]
  Sends a message to the listed windows' message queues
/command [command]
  Describes the command in detail, then exits
/rs or /resize [width] [height]
  resizes the listed windows to match the width and height specified.
```


## Window Wrangler Colors:
Window Wrangler's colors used with the `/col` parameter can be customized by adding/editing the registry keys for them or creating a `Config.txt` file.

Within the `HKEY_CURRENT_USER\SOFTWARE\WindowWrangler` Registry Folder, the following REG_SZ values are checked:
```
Color_Main
Color_Path
Color_PID
Color_Title
Color_Extra
```
These are also checked in the `Config.txt` file.

Their values can, with any capitalization, be:
```
Red,
Green,
Blue,
Magenta,
Yellow,
Cyan
Dark Red, DarkRed, Dark_Red
Dark Green, DarkGreen, Dark_Green
Dark Blue, DarkBlue, Dark_Blue
Dark Magenta, DarkMagenta, Dark_Magenta
Dark Yellow, DarkYellow, Dark_Yellow
Dark Cyan, DarkCyan, Dark_Cyan
White,
Light Gray, Light Grey, Light Grae, Gray, Grey, Grae, LightGray, LightGrey, LightGrae, Light_Gray, Light_Grey, Light_Grae
Dark Gray, Dark Grey, Dark Grae, DarkGray, DarkGrey, DarkGrae, Dark_Gray, Dark_Grey, Dark_Grae
Black
```
Where all the values on a given line refer to the same color. Color names are case-insensitive.
(Yes, I chose to make up a word "Grae" to be deliberately on the fence about how to spell Grae. I'm sorry to any future historians who theorize about if this is a real word or not.)

