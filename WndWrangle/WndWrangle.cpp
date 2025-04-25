// Copyright Ethan M. Hardt 2024 - 2025
// Licensed under the MIT License
// See License.txt for details


#include <Windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>

enum class TextColor { White, LightGrey, DarkGrey, Black, Yellow, DarkYellow, Magenta, DarkMagenta, Red, DarkRed, Cyan, DarkCyan, Green, DarkGreen, Blue, DarkBlue };
enum class TextAttrib { None, Underscore};
HANDLE HCON = nullptr;

void ColorPrint(std::string Str, TextColor Cols = TextColor::White, TextAttrib Atrbs = TextAttrib::None) {
	WORD Properties = 0;
	switch (Cols) {
		case TextColor::White:
			Properties = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case TextColor::LightGrey:
			Properties = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED;
			break;
		case TextColor::DarkGrey:
			Properties = FOREGROUND_INTENSITY;
			break;
		case TextColor::Black:
			// :P
			break;
		case TextColor::Yellow:
			Properties = FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkYellow:
			Properties = FOREGROUND_GREEN | FOREGROUND_RED;
			break;
		case TextColor::Magenta:
			Properties = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkMagenta:
			Properties = FOREGROUND_BLUE | FOREGROUND_RED;
			break;
		case TextColor::Red:
			Properties = FOREGROUND_RED | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkRed:
			Properties = FOREGROUND_RED;
			break;
		case TextColor::Cyan:
			Properties = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkCyan:
			Properties = FOREGROUND_BLUE | FOREGROUND_GREEN;
			break;
		case TextColor::Green:
			Properties = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkGreen:
			Properties = FOREGROUND_GREEN;
			break;
		case TextColor::Blue:
			Properties = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
			break;
		case TextColor::DarkBlue:
			Properties = FOREGROUND_BLUE;
			break;
		default:
			break;
	}
	switch (Atrbs) {
		case TextAttrib::None:
			break;
		case TextAttrib::Underscore:
			Properties |= COMMON_LVB_UNDERSCORE;
			break;
		default:
			break;
	}



	SetConsoleTextAttribute(HCON,Properties);


	std::cout << Str;
}

void EndLine() {
	std::cout << std::endl << std::flush;
}

void PrintLine(std::string Str) {
	ColorPrint(Str);
	EndLine();
}
// Check
uint64_t WindowCount = 0;

// Options:
bool ListHidden						= false;
bool ListUnopened					= false;
bool DisplayPID						= false;
bool CaseSensitive					= true;
bool RepositionWindow				= false;
bool SendWMClose					= false;
bool Color							= false;
std::vector<MSG> MessagesToPost;
std::vector<MSG> MessagesToSend;
unsigned long long int Width		= 0;
unsigned long long int Height		= 0;
bool ResizeWindow					= false;



TextColor C_Main	= TextColor::LightGrey;
TextColor C_Title	= TextColor::Green;
TextColor C_Path	= TextColor::Blue;
TextColor C_PID		= TextColor::Red;
TextColor C_Extra	= TextColor::Magenta;
TextColor C_Count	= TextColor::DarkCyan;

void MaybeColorPrint(std::string Str, TextColor TxCol) {
	if (Color) {
		ColorPrint(Str, TxCol);
	} else {
		ColorPrint(Str, TextColor::White);
	}
}

POINT RepositionPoint;

// Filtering Parameters:
std::vector<std::string> LookFor;
std::vector<DWORD> MatchPID;
std::vector<std::string> nLookFor;
std::vector<DWORD> nMatchPID;

char Lower(char Ch) {
	char ChRet = Ch;
	if (ChRet >= 0x41 && ChRet <= 0x5A) {
		ChRet += 0x20;
	}
	return ChRet;
}

std::string Lower(std::string Str) {
	for (uint64_t A = 0; A < Str.size(); A++) {
		Str[A] = Lower(Str[A]);
	}
	return Str;
}

bool Search(std::string Search, std::string Term) {

	// Exact Match. (probably faster, but I'm not testing it)
	if (Search.size() == Term.size()) {
		if (Search == Term) {
			return true;
		}
	}

	// Partial Match (also Case-insensitive Exact, if enabled)
	if (Search.size() < Term.size()) {
		for (uint64_t A = 0; A < (Term.size() - Search.size()) + 1; A++) {
			bool Success = true;
			for (uint64_t B = 0; B < Search.size(); B++) {
				if (CaseSensitive) {
					if (Search.at(B) != Term.at(B + A)) {
						Success = false;
						break;
					}
				} else {
					if (Lower(Search.at(B)) != Lower(Term.at(B + A))) {
						Success = false;
						break;
					}
				}
			}
			if (Success) {
				return true;
			}
		}
	}

	// No Match.
	return false;
}




BOOL CALLBACK EnumWindowsProc(HWND Window, LPARAM lp) {
	bool SkipEffects = false;
	if (!IsWindowVisible(Window)) {
		if (!ListHidden) {
			return TRUE; // specify -h to list hidden windows.
		}
		SkipEffects = true;
		// Prevents making hidden windows visible.
		// Programs love making hidden windows (can't blame them, especially when making a modern OpenGL context...)
	}
	char Ch[256];
	if (GetWindowTextA(Window, Ch, 256) > 0) {
		std::string WindowTitle = Ch;

		if (WindowTitle == "Program Manager") {
			SkipEffects = true;
			// Added when trying to keep the taskbar (classic start menu) in place, this isn't what was needed, but I feel a terrible fate is coming when thinking about removing it.
		}
		bool Skip = true;
		if (LookFor.size() > 0) {
			for (uint64_t G = 0; G < LookFor.size(); G++) {
				if (Search(LookFor.at(G),WindowTitle)) {
					Skip = false;
					break;
				}
			}
		} else {
			Skip = false;
		}
		if (nLookFor.size() > 0) {
			for (uint64_t G = 0; G < nLookFor.size(); G++) {
				if (Search(nLookFor.at(G),WindowTitle)) {
					Skip = true;
					break;
				}
			}
		}
		if (Skip) {
			return TRUE;
		}
		bool PIDValid = (MatchPID.size() == 0);
		DWORD ProcID;
		if (GetWindowThreadProcessId(Window, &ProcID) != 0) {
			if (MatchPID.size() > 0) {
				for (uint64_t Ind = 0; Ind < MatchPID.size(); Ind++) {
					if (MatchPID.at(Ind) == ProcID) {
						PIDValid = true;
					}
				}
			}
			if (!PIDValid) {
				return TRUE;
			}
			if (nMatchPID.size() > 0) {
				for (uint64_t Ind = 0; Ind < nMatchPID.size(); Ind++) {
					if (nMatchPID.at(Ind) == ProcID) {
						PIDValid = false;
					}
				}
			}
			if (!PIDValid) {
				return TRUE;
			}
			MaybeColorPrint(WindowTitle, C_Title);

			char Ch2[4096];
			/*///
			std::string AdditionalInfo;
			if (DisplayPID) {
				AdditionalInfo += " with PID of " + std::to_string(ProcID);
			}
			//*///

			HANDLE H = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcID);
			if (H != nullptr) {
				DWORD ExSize = 4096;
				if (QueryFullProcessImageNameA(H, 0, Ch2, &ExSize) != 0) {
					std::string ProcessName = Ch2;
					MaybeColorPrint(" belongs to ", C_Main);
					MaybeColorPrint(ProcessName, C_Path);
					//PrintLine(WindowTitle + " Belongs to " + ProcessName + AdditionalInfo);
					CloseHandle(H);
				} else {
					MaybeColorPrint(" belongs to a process with an unknown path", C_Main);
					//PrintLine(WindowTitle + " Belongs to a process we query the name of" + AdditionalInfo);
				}
				WindowCount++;
			} else {
				if (ListUnopened) {
					MaybeColorPrint(" belongs to a process we couldn't open", C_Main);
					//PrintLine(WindowTitle + " Belongs to a process we couldn't open" + AdditionalInfo);
					WindowCount++;
				} else {
					SkipEffects = true;
				}
			}
			if (DisplayPID) {
				MaybeColorPrint(" with PID ", C_Main);
				MaybeColorPrint("#"+std::to_string(ProcID), C_PID);
				//AdditionalInfo += " with PID of " + std::to_string(ProcID);
			}
		} else {
			if (ListUnopened) {
				MaybeColorPrint(WindowTitle, C_Title);
				MaybeColorPrint(" belongs to an unknown process", C_Main);
				//PrintLine(WindowTitle + " Belongs to an unknown process");
			} else {
				SkipEffects = true;
			}
		}
		if (SendWMClose) {
			if (PIDValid) {
				SendMessage(Window, WM_CLOSE, 0, 0);
				SkipEffects = true;
				// Repositioning and Closing should not happen together..
				return TRUE;
			}
		}
		EndLine();
	} else {
		// Prevents moving Classic start menu.
		SkipEffects = true;
	}


	if (!SkipEffects) {
		if (RepositionWindow) {
			SetWindowPos(Window, HWND_TOP, RepositionPoint.x, RepositionPoint.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
			// Hopefully this helps with someone who moved their window offscreen.
		}
		if (ResizeWindow) {
			SetWindowPos(Window, HWND_TOP, 0, 0, int(Width), int(Height), SWP_NOMOVE | SWP_SHOWWINDOW);
		}
		for (unsigned long long Send = 0; Send < MessagesToSend.size(); Send++) {
			MSG& M = MessagesToSend.at(Send);
			PrintLine("Sending Message");
			SendMessage(Window, M.message, M.wParam, M.lParam);
		}
		for (unsigned long long Post = 0; Post < MessagesToPost.size(); Post++) {
			MSG& M = MessagesToPost.at(Post);
			PrintLine("Posting Message");
			PostMessage(Window, M.message, M.wParam, M.lParam);
		}
	}
	return TRUE;
}
TextColor StrToCol(std::string Str) {
	std::string str = Lower(Str);
	TextColor Col = TextColor::White;
	if (str == "red") {
		Col = TextColor::Red;
	} else if (str == "magenta") {
		Col = TextColor::Magenta;
	} else if (str == "blue") {
		Col = TextColor::Blue;
	} else if (str == "cyan") {
		Col = TextColor::Cyan;
	} else if (str == "green") {
		Col = TextColor::Green;
	} else if (str == "yellow") {
		Col = TextColor::Yellow;
	} else if (str == "dark red" || str == "darkred" || str == "dark_red") {
		Col = TextColor::DarkRed;
	} else if (str == "dark magenta" || str == "darkmagenta" || str == "dark_magenta") {
		Col = TextColor::DarkMagenta;
	} else if (str == "dark blue" || str == "darkblue" || str == "dark_blue") {
		Col = TextColor::DarkBlue;
	} else if (str == "dark cyan" || str == "darkcyan" || str == "dark_cyan") {
		Col = TextColor::DarkCyan;
	} else if (str == "dark green" || str == "darkgreen" || str == "dark_green") {
		Col = TextColor::DarkGreen;
	} else if (str == "dark yellow" || str == "darkyellow" || str == "dark_yellow") {
		Col = TextColor::DarkYellow;
	} else if (str == "black") {
		Col = TextColor::Black;
	} else if (str == "dark gray" || str == "dark grey" || str == "dark grae" || str == "darkgray" || str == "darkgrey" || str == "darkgrae" || str == "dark_gray" || str == "dark_grey" || str == "dark_grae") {
		Col = TextColor::DarkGrey;
	} else if (str == "light gray" || str == "light grey" || str == "light grae" || str == "lightgray" || str == "lightgrey" || str == "lightgrae" || str == "gray" || str == "grey" || str == "grae" || str == "light_gray" || str == "light_grey" || str == "light_grae") {
		Col = TextColor::LightGrey;
	} else if (str == "white") {
		Col = TextColor::White;
	}
	return Col;
}

int main(int argc, char* argv[]) {
	HCON = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO CSBI;
	if (HCON != nullptr) {
		if (!GetConsoleScreenBufferInfo(HCON, &CSBI)) {
			HCON = nullptr;
		} else {
			HKEY RegKey;
			if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\WindowWrangler", 0, KEY_READ, &RegKey) == ERROR_SUCCESS) {
				DWORD Type = 0;
				char Char[1024];
				DWORD DataC = 1024;
				std::string Col_Title	= "Green";
				std::string Col_Path	= "Blue";
				std::string Col_PID		= "Red";
				std::string Col_Extra	= "White";
				std::string Col_Main	= "Light Gray";
				std::string Col_Count	= "Dark Cyan";

				DataC = 1024;
				// Color_Extra is a catch-all.
				if (RegGetValue(RegKey, nullptr, "Color_Extra", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Title = Char;
					Col_Path = Char;
					Col_PID = Char;
					Col_Count = Char;
					Col_Extra = Char;
				}

				DataC = 1024;
				// Color_Main is the default color for text.
				if (RegGetValue(RegKey, nullptr, "Color_Main", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Main = Char;
				}

				DataC = 1024;
				// Specialize if Specified.
				if (RegGetValue(RegKey, nullptr, "Color_Title", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Title = Char;
				}
				DataC = 1024;
				if (RegGetValue(RegKey, nullptr, "Color_Path", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Path = Char;
				}
				DataC = 1024;
				if (RegGetValue(RegKey, nullptr, "Color_PID", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_PID = Char;
				}
				DataC = 1024;
				if (RegGetValue(RegKey, nullptr, "Color_Count", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Count = Char;
				}
				std::fstream Conf("Config.txt", std::ios::in);
				if (Conf.good()) {
					std::string ConfigStr;
					while (true) {
						char Ch = Conf.get();
						if (!Conf.good()) {
							Ch = 0x00;
						}
						if (Ch == 0x0A || Ch == 0x0D || Ch == 0x00 || Ch == '.' || Ch == ',' || Ch == ';') {
							std::string StrA;
							std::string StrB;
							bool Switch = false;
							for (unsigned long long StrI = 0; StrI < ConfigStr.size(); StrI++) {
								if (ConfigStr.at(StrI) <= 0x21 || ConfigStr.at(StrI) >= 0x7F) {
									continue;
								}
								if (Switch) {
									StrB += ConfigStr.at(StrI);
								} else {
									if (ConfigStr.at(StrI) == '=' || ConfigStr.at(StrI) == ':') {
										Switch = true;
										continue;
									}
									StrA += ConfigStr.at(StrI);
								}
							}
							for (uint64_t A = 0; A < StrA.size(); A++) {
								StrA.at(A) = Lower(StrA.at(A));
							}
							for (uint64_t A = 0; A < StrB.size(); A++) {
								StrB.at(A) = Lower(StrB.at(A));
							}
							if (StrB != "") {
								if (StrA == "color_extra") {
									Col_Extra = Char;
								}
								if (StrA == "color_main") {
									Col_Main = StrB;
								}
								if (StrA == "color_title") {
									Col_Title = StrB;
								}
								if (StrA == "color_path") {
									Col_Path = StrB;
								}
								if (StrA == "color_pid") {
									Col_PID = StrB;
								}
								if (StrA == "color_count") {
									Col_Count = StrB;
								}
							}

							// Process Line
							ConfigStr = "";
						} else {
							ConfigStr += Ch;
						}
						if (!Conf.good()) {
							break;
						}
					}
				}
				C_Main	= StrToCol(Col_Main);
				C_Title	= StrToCol(Col_Title);
				C_Path	= StrToCol(Col_Path);
				C_PID	= StrToCol(Col_PID);
				C_Count	= StrToCol(Col_Count);
				C_Extra	= StrToCol(Col_Extra);
			}
		}
	}


	for (int argi = 1; argi < argc; argi++) {
		std::string arg = argv[argi];
		std::string argO = arg;
		bool CommandUsedDash = true;
		if (arg.size() > 0) {
			if (arg.at(0) == '/') {
				arg.at(0) = '-';
				CommandUsedDash = false;
			}
		}
		for (uint64_t A = 0; A < arg.size(); A++) {
			arg.at(A) = Lower(arg.at(A));
		}
		bool ValidCommand = false;
		


		if (arg == "-?" || arg == "-help") {

			std::string Denote = "/";
			if (CommandUsedDash) { // These two *should* not both be true in any case...
				Denote = "-";
			}

			PrintLine("Window Wrangler Help:");
			PrintLine("Usage: WndWrangle.exe [switch arguments]");
			PrintLine("");
			PrintLine("for all listed arguments below, you can use either '/' or '-', or both.");
			PrintLine("");
			PrintLine("Information Commands:");
			PrintLine(Denote+"about					Shows the about screen, then exits");
			PrintLine(Denote+"license				Displays the license, then exits");
			PrintLine(Denote+"? or "+Denote+"help				Shows this Help Screen, then exits");
			PrintLine(Denote+"command	[command]		Describes a command in detail, then exits");
			PrintLine("");
			PrintLine("Normal Commands:");
			PrintLine(Denote+"h or "+Denote+"hidden				Lists windows that are hidden");
			PrintLine(Denote+"v or "+Denote+"verbose				Lists windows of unknown processes (Verbose)");
			PrintLine(Denote+"p or "+Denote+"displaypid			Prints the PID");
			PrintLine(Denote+"i or "+Denote+"insensitive			Search is insensitive to case");
			PrintLine(Denote+"r or "+Denote+"reposition			Repositions the listed windows to the mouse (except Program Manager)");
			PrintLine(Denote+"rs or "+Denote+"resize [w h]			Resizes the listed windows with following width/height (except Program Manager)");
			PrintLine(Denote+"col or "+Denote+"color 				Print in Color");
			PrintLine("");
			PrintLine("Filtering Commands:");
			PrintLine(Denote+"f or "+Denote+"find	[title]			Find a window by title");
			PrintLine(Denote+"nf or "+Denote+"negativefind	[title]		Filter window out by title");
			PrintLine(Denote+"m or "+Denote+"match	[PID]			Match a window by program ID");
			PrintLine(Denote+"nm or "+Denote+"negativematch	[PID]		Unmatch a window by program ID");
			PrintLine("");
			PrintLine("Dangerous Commands:			(use with caution)");
			PrintLine(Denote+"c or "+Denote+"close				Send WM_CLOSE to the listed windows. (filter with -f, -nf, and/or -m)");
			PrintLine(Denote+"pm or "+Denote+"postmessage	[msg]		Posts a message to the listed windows message queues");
			PrintLine(Denote+"sm or "+Denote+"sendmessage	[msg]		Sends a message to the listed windows message queues");
			ValidCommand = true;
			return 0;
		}
		if (arg == "-about") {
			PrintLine("Window Wrangler");
			PrintLine("version 1.2.0");
			PrintLine("by Miles (Ethan M. Hardt)");
			PrintLine("(see /authornote)");
			PrintLine("");
			PrintLine("Licensed under MIT");
			PrintLine("See license.txt for the license");
			PrintLine("or use -license");
			PrintLine("(Please do not change the");
			PrintLine("license for downstream distributions)");
			ValidCommand = true;
			return 0;
		}
		if (arg == "-authornote") {
			PrintLine("Author's Note:");
			PrintLine("I prefer to be called by \"Miles\".");
			PrintLine("unless you need to do otherwise, Please do not use my first or last name.");
			PrintLine("My full legal name is included specifically for legal reasons.");
			ValidCommand = true;
			return 0;
		}
		if (arg == "-license") {
			PrintLine("MIT License");
			PrintLine("");
			PrintLine("Copyright (c) 2024 - 2025 Ethan Miles Hardt");
			PrintLine("");
			PrintLine("Permission is hereby granted, free of charge, to any person obtaining a copy");
			PrintLine("of this software and associated documentation files (the \"Software\"), to deal");
			PrintLine("in the Software without restriction, including without limitation the rights");
			PrintLine("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell");
			PrintLine("copies of the Software, and to permit persons to whom the Software is");
			PrintLine("furnished to do so, subject to the following conditions:");
			PrintLine("");
			PrintLine("The above copyright notice and this permission notice shall be included in all");
			PrintLine("copies or substantial portions of the Software.");
			PrintLine("");
			PrintLine("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
			PrintLine("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
			PrintLine("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
			PrintLine("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
			PrintLine("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
			PrintLine("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
			PrintLine("SOFTWARE.");
			ValidCommand = true;
			return 0;
		}
		if (arg == "-command") {
			if (argi + 1 < argc) {
				argi++;
				std::string Command = argv[argi];
				bool InitialDash = Command.at(0) == '-';
				bool InitialSlash = Command.at(0) == '/';
				if (Command.size() > 0) {
					if (Command.at(0) == '/' || Command.at(0) == '-') {
						Command = Command.substr(1);
					}
				}
				std::string Denote = "/";
				if (InitialDash && !InitialSlash) { // These two *should* not both be true in any case...
					Denote = "-";
				}
				for (uint64_t A = 0; A < Command.size(); A++) {
					Command.at(A) = Lower(Command.at(A));
				}
				bool Valid = false;
				if (Command == "about" || Command == "*") {
					PrintLine("Command: "+Denote+"about");
					PrintLine("Gives a short informational screen with the program name and version, then exits.");
					PrintLine("also names the MIT license for the source code.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "authornote" || Command == "*") {
					PrintLine("Command: "+Denote+"authornote");
					PrintLine("Prints a personal request, then exits.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "license" || Command == "*") {
					PrintLine("Command: "+Denote+"license");
					PrintLine("Displays the MIT License, then exits.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "help" || Command == "?" || Command == "*") {
					PrintLine("Command: "+Denote+"? | "+Denote+"help");
					PrintLine("Displays a list of commands and summarizes their function, then exits.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "command" || Command == "*") {
					PrintLine("Command: "+Denote+"command [CommandName]");
					PrintLine("Gives a short description of the command specified by [CommandName], then exits.");
					if (Command != "*") {
						PrintLine("You can also specify `*` to print out information on all commands.");
					} else {
						PrintLine("Oh cool, you did the `*` thing.");
					}
					PrintLine("");
					Valid = true;
				}
				if (Command == "h" || Command == "hidden" || Command == "*") {
					PrintLine("Command: "+Denote+"h | "+Denote+"hidden");
					PrintLine("lists hidden windows.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "v" || Command == "verbose" || Command == "*") {
					PrintLine("Command: "+Denote+"v | "+Denote+"verbose");
					PrintLine("lists windows of unknown processes.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "p" || Command == "displaypid" || Command == "*") {
					PrintLine("Command: "+Denote+"p | "+Denote+"displaypid");
					PrintLine("Displays the PID of each window listed.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "i" || Command == "insensitive" || Command == "*") {
					PrintLine("Command: "+Denote+"i | "+Denote+"insensitive");
					PrintLine("Makes filtering by title case-insensitive.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "c" || Command == "close" || Command == "*") {
					PrintLine("Command: "+Denote+"c | "+Denote+"close");
					PrintLine("Closes a window, particularly by sending WM_CLOSE.");
					PrintLine("This effectively does the same thing as clicking the close button.");
					PrintLine("A window can ignore it however.");
					PrintLine("Note: Using this option to close a window will ignore all other specified effects.");
					PrintLine("you can use -sm or -pm if other effects are needed.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "r" || Command == "reposition" || Command == "*") {
					PrintLine("Command: "+Denote+"r | "+Denote+"reposition");
					PrintLine("Repositions a window to have its top-left corner at the mouse.");
					PrintLine("this does not update the Z-order or focus of the window.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "rs" || Command == "resize" || Command == "*") {
					PrintLine("Command: "+Denote+"rs [W, H] | "+Denote+"resize [W, H]");
					PrintLine("Resizes a window to have a new size determined by parameters [W] for width and [H] for height.");
					PrintLine("this does not update the Z-order or focus of the window.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "col" || Command == "color" || Command == "colorized" || Command == "colorify" || Command == "*") {
					PrintLine("Command:"+Denote+"col | "+Denote+"color |"+Denote+"colorized |"+Denote+"colorify");
					PrintLine("Prints the list of windows in color, specified by registry or local config.txt file.");
					PrintLine("The local config file is prioritized over the registry.");
					if (Color) {
						ColorPrint("C",TextColor::Red);
						ColorPrint("O",TextColor::Yellow);
						ColorPrint("L",TextColor::Green);
						ColorPrint("O",TextColor::Cyan);
						ColorPrint("R",TextColor::Blue);
						ColorPrint("S",TextColor::Magenta);
					}
					PrintLine("");
					PrintLine("");
					Valid = true;
				}
				if (Command == "f" || Command == "find" || Command == "*") {
					PrintLine("Command:"+Denote+"f [Name] |"+Denote+"find [Name]");
					PrintLine("Finds windows that partially match the parameter.");
					PrintLine("Normally case sensitive.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "nf" || Command == "negativefind" || Command == "*") {
					PrintLine("Command:"+Denote+"nf [Name] |"+Denote+"negativefind [Name]");
					PrintLine("Filters out windows that partially match the parameter.");
					PrintLine("Normally case sensitive.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "m" || Command == "match" || Command == "*") {
					PrintLine("Command:"+Denote+"m [PID] |"+Denote+"match [PID]");
					PrintLine("matches windows that match PID with the parameter.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "nm" || Command == "negativematch" || Command == "*") {
					PrintLine("Command:"+Denote+"nm [Name] |"+Denote+"negativematch [Name]");
					PrintLine("Filters out windows that match PID with the parameter.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "pm" || Command == "pushmessage" || Command == "*") {
					PrintLine("Command:"+Denote+"pm [MessageType, WideParam, LongParam] |"+Denote+"pushmessage [MessageType, WideParam, LongParam]");
					PrintLine("Resolves the MessageType to a MessageID, if it is a `WM_` Constant Name.");
					PrintLine("The [WideParam] and [LongParam] must be numbers.");
					PrintLine("Please refer to the MSDN or any other accurate documentation for how to use any given `WM_` Constant.");
					PrintLine("Pushes the MessageID to the window, queueing it for processing.");
					PrintLine("");
					Valid = true;
				}
				if (Command == "sm" || Command == "sendmessage" || Command == "*") {
					PrintLine("Command:"+Denote+"sm [MessageType, WideParam, LongParam] |"+Denote+"sendmessage [MessageType, WideParam, LongParam]");
					PrintLine("Resolves the MessageType to a MessageID, if it is a `WM_` Constant Name.");
					PrintLine("The [WideParam] and [LongParam] must be numbers.");
					PrintLine("Please refer to the MSDN or any other accurate documentation for how to use any given `WM_` Constant.");
					PrintLine("Sends the MessageID to the window, processing it immediately.");
					PrintLine("");
					Valid = true;
				}
				if (!Valid) {
					if (InitialDash) { Command = "-" + Command; }
					if (InitialSlash) { Command = "/" + Command; }
					PrintLine("Command "+Command);
					PrintLine("Unknown Command.");
				}
				ValidCommand = true;
				return 0;
			}
			PrintLine("No Command Specified");
			ValidCommand = true;
			return 0;
		}
		if (arg == "-pm" || arg == "-postmessage") {
			if (argi + 3 < argc) {
				MSG ConstructMessage;
				ConstructMessage.hwnd = nullptr;

				std::string MessageTypeStr	= argv[argi + 1];
				std::string WideParamStr	= argv[argi + 2];
				std::string LongParamStr	= argv[argi + 3];


				try {
					ConstructMessage.message = std::stoi(MessageTypeStr);
					argi += 3;
				} catch (...) {
					for (uint64_t A = 0; A < MessageTypeStr.size(); A++) {
						MessageTypeStr.at(A) = Lower(MessageTypeStr.at(A));
					}
					if (MessageTypeStr.substr(0, 3) == "wm_" && MessageTypeStr.size() > 3) {
						MessageTypeStr = MessageTypeStr.substr(3);
					}
					// match message to constant
					bool Valid = false;

					// C++ doesn't have string switching, so...
					if (true) {
						if (MessageTypeStr == "activateapp") {
							ConstructMessage.message = WM_ACTIVATEAPP;
							Valid = true;
						} else if (MessageTypeStr == "cancelmode") {
							ConstructMessage.message = WM_CANCELMODE;
							Valid = true;
						} else if (MessageTypeStr == "childactivate") {
							ConstructMessage.message = WM_CHILDACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "close") {
							ConstructMessage.message = WM_CLOSE;
							Valid = true;
						} else if (MessageTypeStr == "compacting") {
							ConstructMessage.message = WM_COMPACTING;
							Valid = true;
						} else if (MessageTypeStr == "create") {
							ConstructMessage.message = WM_CREATE;
							Valid = true;
						} else if (MessageTypeStr == "destroy") {
							ConstructMessage.message = WM_DESTROY;
							Valid = true;
						} else if (MessageTypeStr == "dpichanged") {
							ConstructMessage.message = WM_DPICHANGED;
							Valid = true;
						} else if (MessageTypeStr == "enable") {
							ConstructMessage.message = WM_ENABLE;
							Valid = true;
						} else if (MessageTypeStr == "entersizemove") {
							ConstructMessage.message = WM_ENTERSIZEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "exitsizemove") {
							ConstructMessage.message = WM_EXITSIZEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "geticon") {
							ConstructMessage.message = WM_GETICON;
							Valid = true;
						} else if (MessageTypeStr == "getminmaxinfo") {
							ConstructMessage.message = WM_GETMINMAXINFO;
							Valid = true;
						} else if (MessageTypeStr == "inputlangchange") {
							ConstructMessage.message = WM_INPUTLANGCHANGE;
							Valid = true;
						} else if (MessageTypeStr == "inputlangchangerequest") {
							ConstructMessage.message = WM_INPUTLANGCHANGEREQUEST;
							Valid = true;
						} else if (MessageTypeStr == "move") {
							ConstructMessage.message = WM_MOVE;
							Valid = true;
						} else if (MessageTypeStr == "moving") {
							ConstructMessage.message = WM_MOVING;
							Valid = true;
						} else if (MessageTypeStr == "ncactivate") {
							ConstructMessage.message = WM_NCACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "nccalcsize") {
							ConstructMessage.message = WM_NCCALCSIZE;
							Valid = true;
						} else if (MessageTypeStr == "nccreate") {
							ConstructMessage.message = WM_NCCREATE;
							Valid = true;
						} else if (MessageTypeStr == "ncdestroy") {
							ConstructMessage.message = WM_NCDESTROY;
							Valid = true;
						} else if (MessageTypeStr == "null") {
							ConstructMessage.message = WM_NULL;
							Valid = true;
						} else if (MessageTypeStr == "querydragicon") {
							ConstructMessage.message = WM_QUERYDRAGICON;
							Valid = true;
						} else if (MessageTypeStr == "queryopen") {
							ConstructMessage.message = WM_QUERYOPEN;
							Valid = true;
						} else if (MessageTypeStr == "quit") {
							ConstructMessage.message = WM_QUIT;
							Valid = true;
						} else if (MessageTypeStr == "showwindow") {
							ConstructMessage.message = WM_SHOWWINDOW;
							Valid = true;
						} else if (MessageTypeStr == "size") {
							ConstructMessage.message = WM_SIZE;
							Valid = true;
						} else if (MessageTypeStr == "sizing") {
							ConstructMessage.message = WM_SIZING;
							Valid = true;
						} else if (MessageTypeStr == "stylechanged") {
							ConstructMessage.message = WM_STYLECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "stylechanging") {
							ConstructMessage.message = WM_STYLECHANGING;
							Valid = true;
						} else if (MessageTypeStr == "themechanged") {
							ConstructMessage.message = WM_THEMECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "userchanged") {
							ConstructMessage.message = WM_USERCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "windowposchanged") {
							ConstructMessage.message = WM_WINDOWPOSCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "windowposchanging") {
							ConstructMessage.message = WM_WINDOWPOSCHANGING;
							Valid = true;
						// a non-'WM_' constant.
						} else if (MessageTypeStr == "erasebkgnd") {
							ConstructMessage.message = WM_ERASEBKGND;
							Valid = true;
						} else if (MessageTypeStr == "getfont") {
							ConstructMessage.message = WM_GETFONT;
							Valid = true;
						} else if (MessageTypeStr == "gettext") {
							ConstructMessage.message = WM_GETTEXT;
							Valid = true;
						} else if (MessageTypeStr == "gettextlength") {
							ConstructMessage.message = WM_GETTEXTLENGTH;
							Valid = true;
						} else if (MessageTypeStr == "setfont") {
							ConstructMessage.message = WM_SETFONT;
							Valid = true;
						} else if (MessageTypeStr == "seticon") {
							ConstructMessage.message = WM_SETICON;
							Valid = true;
						} else if (MessageTypeStr == "settext") {
							ConstructMessage.message = WM_SETTEXT;
							Valid = true;
						} else if (MessageTypeStr == "timer") {
							ConstructMessage.message = WM_TIMER;
							Valid = true;
						} else if (MessageTypeStr == "ctlcolorscrollbar") {
							ConstructMessage.message = WM_CTLCOLORSCROLLBAR;
							Valid = true;
						} else if (MessageTypeStr == "hscroll") {
							ConstructMessage.message = WM_HSCROLL;
							Valid = true;
						} else if (MessageTypeStr == "vscroll") {
							ConstructMessage.message = WM_VSCROLL;
							Valid = true;
						} else if (MessageTypeStr == "input") {
							ConstructMessage.message = WM_INPUT;
							Valid = true;
						} else if (MessageTypeStr == "input_device_change") {
							ConstructMessage.message = WM_INPUT_DEVICE_CHANGE;
							Valid = true;
						} else if (MessageTypeStr == "mdiactivate") {
							ConstructMessage.message = WM_MDIACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "mdicascade") {
							ConstructMessage.message = WM_MDICASCADE;
							Valid = true;
						} else if (MessageTypeStr == "mdicreate") {
							ConstructMessage.message = WM_MDICREATE;
							Valid = true;
						} else if (MessageTypeStr == "mdidestroy") {
							ConstructMessage.message = WM_MDIDESTROY;
							Valid = true;
						} else if (MessageTypeStr == "mdigetactive") {
							ConstructMessage.message = WM_MDIGETACTIVE;
							Valid = true;
						} else if (MessageTypeStr == "mdiiconarrange") {
							ConstructMessage.message = WM_MDIICONARRANGE;
							Valid = true;
						} else if (MessageTypeStr == "mdimaximize") {
							ConstructMessage.message = WM_MDIMAXIMIZE;
							Valid = true;
						} else if (MessageTypeStr == "mdinext") {
							ConstructMessage.message = WM_MDINEXT;
							Valid = true;
						} else if (MessageTypeStr == "mdirefreshmenu") {
							ConstructMessage.message = WM_MDIREFRESHMENU;
							Valid = true;
						} else if (MessageTypeStr == "mdirestore") {
							ConstructMessage.message = WM_MDIRESTORE;
							Valid = true;
						} else if (MessageTypeStr == "mdisetmenu") {
							ConstructMessage.message = WM_MDISETMENU;
							Valid = true;
						} else if (MessageTypeStr == "mditile") {
							ConstructMessage.message = WM_MDITILE;
							Valid = true;
						} else if (MessageTypeStr == "capturechanged") {
							ConstructMessage.message = WM_CAPTURECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "lbuttondblclk") {
							ConstructMessage.message = WM_LBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "lbuttondown") {
							ConstructMessage.message = WM_LBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "lbuttonup") {
							ConstructMessage.message = WM_LBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "mbuttondblclk") {
							ConstructMessage.message = WM_MBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "mbuttondown") {
							ConstructMessage.message = WM_MBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "mbuttonup") {
							ConstructMessage.message = WM_MBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "mouseactivate") {
							ConstructMessage.message = WM_MOUSEACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "mousehover") {
							ConstructMessage.message = WM_MOUSEHOVER;
							Valid = true;
						} else if (MessageTypeStr == "mousehwheel") {
							ConstructMessage.message = WM_MOUSEHWHEEL;
							Valid = true;
						} else if (MessageTypeStr == "mouseleave") {
							ConstructMessage.message = WM_MOUSELEAVE;
							Valid = true;
						} else if (MessageTypeStr == "mousemove") {
							ConstructMessage.message = WM_MOUSEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "mousewheel") {
							ConstructMessage.message = WM_MOUSEWHEEL;
							Valid = true;
						} else if (MessageTypeStr == "nchittest") {
							ConstructMessage.message = WM_NCHITTEST;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttondblclk") {
							ConstructMessage.message = WM_NCLBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttondown") {
							ConstructMessage.message = WM_NCLBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttonup") {
							ConstructMessage.message = WM_NCLBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttondblclk") {
							ConstructMessage.message = WM_NCMBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttondown") {
							ConstructMessage.message = WM_NCMBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttonup") {
							ConstructMessage.message = WM_NCMBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncmousehover") {
							ConstructMessage.message = WM_NCMOUSEHOVER;
							Valid = true;
						} else if (MessageTypeStr == "ncmouseleave") {
							ConstructMessage.message = WM_NCMOUSELEAVE;
							Valid = true;
						} else if (MessageTypeStr == "ncmousemove") {
							ConstructMessage.message = WM_NCMOUSEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttondblclk") {
							ConstructMessage.message = WM_NCRBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttondown") {
							ConstructMessage.message = WM_NCRBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttonup") {
							ConstructMessage.message = WM_NCRBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttondblclk") {
							ConstructMessage.message = WM_NCXBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttondown") {
							ConstructMessage.message = WM_NCXBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttonup") {
							ConstructMessage.message = WM_NCXBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "rbuttondblclk") {
							ConstructMessage.message = WM_RBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "rbuttondown") {
							ConstructMessage.message = WM_RBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "rbuttonup") {
							ConstructMessage.message = WM_RBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "xbuttondblclk") {
							ConstructMessage.message = WM_XBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "xbuttondown") {
							ConstructMessage.message = WM_XBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "xbuttonup") {
							ConstructMessage.message = WM_XBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "command") {
							ConstructMessage.message = WM_COMMAND;
							Valid = true;
						} else if (MessageTypeStr == "contextmenu") {
							ConstructMessage.message = WM_CONTEXTMENU;
							Valid = true;
						} else if (MessageTypeStr == "entermenuloop") {
							ConstructMessage.message = WM_ENTERMENULOOP;
							Valid = true;
						} else if (MessageTypeStr == "exitmenuloop") {
							ConstructMessage.message = WM_EXITMENULOOP;
							Valid = true;
						} else if (MessageTypeStr == "gettitlebarinfoex") {
							ConstructMessage.message = WM_GETTITLEBARINFOEX;
							Valid = true;
						} else if (MessageTypeStr == "menucommand") {
							ConstructMessage.message = WM_MENUCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "menudrag") {
							ConstructMessage.message = WM_MENUDRAG;
							Valid = true;
						} else if (MessageTypeStr == "menugetobject") {
							ConstructMessage.message = WM_MENUGETOBJECT;
							Valid = true;
						} else if (MessageTypeStr == "menurbuttonup") {
							ConstructMessage.message = WM_MENURBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "nextmenu") {
							ConstructMessage.message = WM_NEXTMENU;
							Valid = true;
						} else if (MessageTypeStr == "uninitmenupopup") {
							ConstructMessage.message = WM_UNINITMENUPOPUP;
							Valid = true;
						} else if (MessageTypeStr == "activate") {
							ConstructMessage.message = WM_ACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "appcommand") {
							ConstructMessage.message = WM_APPCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "char") {
							ConstructMessage.message = WM_CHAR;
							Valid = true;
						} else if (MessageTypeStr == "deadchar") {
							ConstructMessage.message = WM_DEADCHAR;
							Valid = true;
						} else if (MessageTypeStr == "hotkey") {
							ConstructMessage.message = WM_HOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "keydown") {
							ConstructMessage.message = WM_KEYDOWN;
							Valid = true;
						} else if (MessageTypeStr == "keyup") {
							ConstructMessage.message = WM_KEYUP;
							Valid = true;
						} else if (MessageTypeStr == "killfocus") {
							ConstructMessage.message = WM_KILLFOCUS;
							Valid = true;
						} else if (MessageTypeStr == "setfocus") {
							ConstructMessage.message = WM_SETFOCUS;
							Valid = true;
						} else if (MessageTypeStr == "sysdeadchar") {
							ConstructMessage.message = WM_SYSDEADCHAR;
							Valid = true;
						} else if (MessageTypeStr == "syskeydown") {
							ConstructMessage.message = WM_SYSKEYDOWN;
							Valid = true;
						}
						// MSVC Complained about this being too nested.
						if (MessageTypeStr == "syskeyup") {
							ConstructMessage.message = WM_SYSKEYUP;
							Valid = true;
						} else if (MessageTypeStr == "unichar") {
							ConstructMessage.message = WM_UNICHAR;
							Valid = true;
						} else if (MessageTypeStr == "gethotkey") {
							ConstructMessage.message = WM_GETHOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "sethotkey") {
							ConstructMessage.message = WM_SETHOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "initmenupopup") {
							ConstructMessage.message = WM_INITMENUPOPUP;
							Valid = true;
						} else if (MessageTypeStr == "menuchar") {
							ConstructMessage.message = WM_MENUCHAR;
							Valid = true;
						} else if (MessageTypeStr == "menuselect") {
							ConstructMessage.message = WM_MENUSELECT;
							Valid = true;
						} else if (MessageTypeStr == "syschar") {
							ConstructMessage.message = WM_SYSCHAR;
							Valid = true;
						} else if (MessageTypeStr == "syscommand") {
							ConstructMessage.message = WM_SYSCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "changeuistate") {
							ConstructMessage.message = WM_CHANGEUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "initmenu") {
							ConstructMessage.message = WM_INITMENU;
							Valid = true;
						} else if (MessageTypeStr == "queryuistate") {
							ConstructMessage.message = WM_QUERYUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "updateuistate") {
							ConstructMessage.message = WM_UPDATEUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "canceljournal") {
							ConstructMessage.message = WM_CANCELJOURNAL;
							Valid = true;
						} else if (MessageTypeStr == "queuesync") {
							ConstructMessage.message = WM_QUEUESYNC;
							Valid = true;
						} else if (MessageTypeStr == "dde_ack") {
							ConstructMessage.message = WM_DDE_ACK;
							Valid = true;
						} else if (MessageTypeStr == "dde_advise") {
							ConstructMessage.message = WM_DDE_ADVISE;
							Valid = true;
						} else if (MessageTypeStr == "dde_execute") {
							ConstructMessage.message = WM_DDE_EXECUTE;
							Valid = true;
						} else if (MessageTypeStr == "dde_poke") {
							ConstructMessage.message = WM_DDE_POKE;
							Valid = true;
						} else if (MessageTypeStr == "dde_request") {
							ConstructMessage.message = WM_DDE_REQUEST;
							Valid = true;
						} else if (MessageTypeStr == "dde_terminate") {
							ConstructMessage.message = WM_DDE_TERMINATE;
							Valid = true;
						} else if (MessageTypeStr == "dde_unadvise") {
							ConstructMessage.message = WM_DDE_UNADVISE;
							Valid = true;
						} else if (MessageTypeStr == "dde_initiate") {
							ConstructMessage.message = WM_DDE_INITIATE;
							Valid = true;
						} else if (MessageTypeStr == "ctlcolordlg") {
							ConstructMessage.message = WM_CTLCOLORDLG;
							Valid = true;
						} else if (MessageTypeStr == "enteridle") {
							ConstructMessage.message = WM_ENTERIDLE;
							Valid = true;
						} else if (MessageTypeStr == "getdlgcode") {
							ConstructMessage.message = WM_GETDLGCODE;
							Valid = true;
						} else if (MessageTypeStr == "initdialog") {
							ConstructMessage.message = WM_INITDIALOG;
							Valid = true;
						} else if (MessageTypeStr == "nextdlgctl") {
							ConstructMessage.message = WM_NEXTDLGCTL;
							Valid = true;
						} else if (MessageTypeStr == "devicechange") {
							ConstructMessage.message = WM_DEVICECHANGE;
							Valid = true;
						} else if (MessageTypeStr == "dwmcolorizationcolorchanged") {
							ConstructMessage.message = WM_DWMCOLORIZATIONCOLORCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmcompositionchanged") {
							ConstructMessage.message = WM_DWMCOMPOSITIONCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmncrenderingchanged") {
							ConstructMessage.message = WM_DWMNCRENDERINGCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmsendiconiclivepreviewbitmap") {
							ConstructMessage.message = WM_DWMSENDICONICLIVEPREVIEWBITMAP;
							Valid = true;
						} else if (MessageTypeStr == "dwmsendiconicthumbnail") {
							ConstructMessage.message = WM_DWMSENDICONICTHUMBNAIL;
							Valid = true;
						} else if (MessageTypeStr == "dwmwindowmaximizedchange") {
							ConstructMessage.message = WM_DWMWINDOWMAXIMIZEDCHANGE;
							Valid = true;
						} else if (MessageTypeStr == "copydata") {
							ConstructMessage.message = WM_COPYDATA;
							Valid = true;
						} else if (MessageTypeStr == "setcursor") {
							ConstructMessage.message = WM_SETCURSOR;
							Valid = true;

							// Constants that do not begin with 'WM_'

							// quite a few of those constants

							// so many non 'WM_' constants

						} else if (MessageTypeStr == "askcbformatname") {
							ConstructMessage.message = WM_ASKCBFORMATNAME;
							Valid = true;
						} else if (MessageTypeStr == "changecbchain") {
							ConstructMessage.message = WM_CHANGECBCHAIN;
							Valid = true;
						} else if (MessageTypeStr == "clipboardupdate") {
							ConstructMessage.message = WM_CLIPBOARDUPDATE;
							Valid = true;
						} else if (MessageTypeStr == "destroyclipboard") {
							ConstructMessage.message = WM_DESTROYCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "drawclipboard") {
							ConstructMessage.message = WM_DRAWCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "hscrollclipboard") {
							ConstructMessage.message = WM_HSCROLLCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "paintclipboard") {
							ConstructMessage.message = WM_PAINTCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "renderformat") {
							ConstructMessage.message = WM_RENDERFORMAT;
							Valid = true;
						} else if (MessageTypeStr == "sizeclipboard") {
							ConstructMessage.message = WM_SIZECLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "vscrollclipboard") {
							ConstructMessage.message = WM_VSCROLLCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "clear") {
							ConstructMessage.message = WM_CLEAR;
							Valid = true;
						} else if (MessageTypeStr == "copy") {
							ConstructMessage.message = WM_COPY;
							Valid = true;
						} else if (MessageTypeStr == "cut") {
							ConstructMessage.message = WM_CUT;
							Valid = true;
						} else if (MessageTypeStr == "paste") {
							ConstructMessage.message = WM_PASTE;
							Valid = true;
						} else if (MessageTypeStr == "undo") {
							ConstructMessage.message = WM_UNDO;
							Valid = true;
						}
					}
					

					if (Valid) {
						argi += 3;
					} else {
						PrintLine("Could not interpret Message");
						return -1;
					}
				}
				try {
					ConstructMessage.wParam = std::stoi(WideParamStr);
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a wParam Argument");
					return -1;
				}
				try {
					ConstructMessage.lParam = std::stoi(LongParamStr);
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to an lParam Argument");
					return -1;
				}
				MessagesToPost.push_back(ConstructMessage);
				ValidCommand = true;
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-sm" || arg == "-sendmessage") {
			if (argi + 3 < argc) {
				MSG ConstructMessage;
				ConstructMessage.hwnd = nullptr;

				std::string MessageTypeStr	= argv[argi + 1];
				std::string WideParamStr	= argv[argi + 2];
				std::string LongParamStr	= argv[argi + 3];


				try {
					ConstructMessage.message = std::stoi(MessageTypeStr);
					argi += 3;
				} catch (...) {
					for (uint64_t A = 0; A < MessageTypeStr.size(); A++) {
						MessageTypeStr.at(A) = Lower(MessageTypeStr.at(A));
					}
					if (MessageTypeStr.substr(0, 3) == "wm_" && MessageTypeStr.size() > 3) {
						MessageTypeStr = MessageTypeStr.substr(3);
					}
					// match message to constant
					bool Valid = false;

					// C++ doesn't have string switching, so...
					if (true) {
						if (MessageTypeStr == "activateapp") {
							ConstructMessage.message = WM_ACTIVATEAPP;
							Valid = true;
						} else if (MessageTypeStr == "cancelmode") {
							ConstructMessage.message = WM_CANCELMODE;
							Valid = true;
						} else if (MessageTypeStr == "childactivate") {
							ConstructMessage.message = WM_CHILDACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "close") {
							ConstructMessage.message = WM_CLOSE;
							Valid = true;
						} else if (MessageTypeStr == "compacting") {
							ConstructMessage.message = WM_COMPACTING;
							Valid = true;
						} else if (MessageTypeStr == "create") {
							ConstructMessage.message = WM_CREATE;
							Valid = true;
						} else if (MessageTypeStr == "destroy") {
							ConstructMessage.message = WM_DESTROY;
							Valid = true;
						} else if (MessageTypeStr == "dpichanged") {
							ConstructMessage.message = WM_DPICHANGED;
							Valid = true;
						} else if (MessageTypeStr == "enable") {
							ConstructMessage.message = WM_ENABLE;
							Valid = true;
						} else if (MessageTypeStr == "entersizemove") {
							ConstructMessage.message = WM_ENTERSIZEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "exitsizemove") {
							ConstructMessage.message = WM_EXITSIZEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "geticon") {
							ConstructMessage.message = WM_GETICON;
							Valid = true;
						} else if (MessageTypeStr == "getminmaxinfo") {
							ConstructMessage.message = WM_GETMINMAXINFO;
							Valid = true;
						} else if (MessageTypeStr == "inputlangchange") {
							ConstructMessage.message = WM_INPUTLANGCHANGE;
							Valid = true;
						} else if (MessageTypeStr == "inputlangchangerequest") {
							ConstructMessage.message = WM_INPUTLANGCHANGEREQUEST;
							Valid = true;
						} else if (MessageTypeStr == "move") {
							ConstructMessage.message = WM_MOVE;
							Valid = true;
						} else if (MessageTypeStr == "moving") {
							ConstructMessage.message = WM_MOVING;
							Valid = true;
						} else if (MessageTypeStr == "ncactivate") {
							ConstructMessage.message = WM_NCACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "nccalcsize") {
							ConstructMessage.message = WM_NCCALCSIZE;
							Valid = true;
						} else if (MessageTypeStr == "nccreate") {
							ConstructMessage.message = WM_NCCREATE;
							Valid = true;
						} else if (MessageTypeStr == "ncdestroy") {
							ConstructMessage.message = WM_NCDESTROY;
							Valid = true;
						} else if (MessageTypeStr == "null") {
							ConstructMessage.message = WM_NULL;
							Valid = true;
						} else if (MessageTypeStr == "querydragicon") {
							ConstructMessage.message = WM_QUERYDRAGICON;
							Valid = true;
						} else if (MessageTypeStr == "queryopen") {
							ConstructMessage.message = WM_QUERYOPEN;
							Valid = true;
						} else if (MessageTypeStr == "quit") {
							ConstructMessage.message = WM_QUIT;
							Valid = true;
						} else if (MessageTypeStr == "showwindow") {
							ConstructMessage.message = WM_SHOWWINDOW;
							Valid = true;
						} else if (MessageTypeStr == "size") {
							ConstructMessage.message = WM_SIZE;
							Valid = true;
						} else if (MessageTypeStr == "sizing") {
							ConstructMessage.message = WM_SIZING;
							Valid = true;
						} else if (MessageTypeStr == "stylechanged") {
							ConstructMessage.message = WM_STYLECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "stylechanging") {
							ConstructMessage.message = WM_STYLECHANGING;
							Valid = true;
						} else if (MessageTypeStr == "themechanged") {
							ConstructMessage.message = WM_THEMECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "userchanged") {
							ConstructMessage.message = WM_USERCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "windowposchanged") {
							ConstructMessage.message = WM_WINDOWPOSCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "windowposchanging") {
							ConstructMessage.message = WM_WINDOWPOSCHANGING;
							Valid = true;
						// a non-'WM_' constant.
						} else if (MessageTypeStr == "erasebkgnd") {
							ConstructMessage.message = WM_ERASEBKGND;
							Valid = true;
						} else if (MessageTypeStr == "getfont") {
							ConstructMessage.message = WM_GETFONT;
							Valid = true;
						} else if (MessageTypeStr == "gettext") {
							ConstructMessage.message = WM_GETTEXT;
							Valid = true;
						} else if (MessageTypeStr == "gettextlength") {
							ConstructMessage.message = WM_GETTEXTLENGTH;
							Valid = true;
						} else if (MessageTypeStr == "setfont") {
							ConstructMessage.message = WM_SETFONT;
							Valid = true;
						} else if (MessageTypeStr == "seticon") {
							ConstructMessage.message = WM_SETICON;
							Valid = true;
						} else if (MessageTypeStr == "settext") {
							ConstructMessage.message = WM_SETTEXT;
							Valid = true;
						} else if (MessageTypeStr == "timer") {
							ConstructMessage.message = WM_TIMER;
							Valid = true;
						} else if (MessageTypeStr == "ctlcolorscrollbar") {
							ConstructMessage.message = WM_CTLCOLORSCROLLBAR;
							Valid = true;
						} else if (MessageTypeStr == "hscroll") {
							ConstructMessage.message = WM_HSCROLL;
							Valid = true;
						} else if (MessageTypeStr == "vscroll") {
							ConstructMessage.message = WM_VSCROLL;
							Valid = true;
						} else if (MessageTypeStr == "input") {
							ConstructMessage.message = WM_INPUT;
							Valid = true;
						} else if (MessageTypeStr == "input_device_change") {
							ConstructMessage.message = WM_INPUT_DEVICE_CHANGE;
							Valid = true;
						} else if (MessageTypeStr == "mdiactivate") {
							ConstructMessage.message = WM_MDIACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "mdicascade") {
							ConstructMessage.message = WM_MDICASCADE;
							Valid = true;
						} else if (MessageTypeStr == "mdicreate") {
							ConstructMessage.message = WM_MDICREATE;
							Valid = true;
						} else if (MessageTypeStr == "mdidestroy") {
							ConstructMessage.message = WM_MDIDESTROY;
							Valid = true;
						} else if (MessageTypeStr == "mdigetactive") {
							ConstructMessage.message = WM_MDIGETACTIVE;
							Valid = true;
						} else if (MessageTypeStr == "mdiiconarrange") {
							ConstructMessage.message = WM_MDIICONARRANGE;
							Valid = true;
						} else if (MessageTypeStr == "mdimaximize") {
							ConstructMessage.message = WM_MDIMAXIMIZE;
							Valid = true;
						} else if (MessageTypeStr == "mdinext") {
							ConstructMessage.message = WM_MDINEXT;
							Valid = true;
						} else if (MessageTypeStr == "mdirefreshmenu") {
							ConstructMessage.message = WM_MDIREFRESHMENU;
							Valid = true;
						} else if (MessageTypeStr == "mdirestore") {
							ConstructMessage.message = WM_MDIRESTORE;
							Valid = true;
						} else if (MessageTypeStr == "mdisetmenu") {
							ConstructMessage.message = WM_MDISETMENU;
							Valid = true;
						} else if (MessageTypeStr == "mditile") {
							ConstructMessage.message = WM_MDITILE;
							Valid = true;
						} else if (MessageTypeStr == "capturechanged") {
							ConstructMessage.message = WM_CAPTURECHANGED;
							Valid = true;
						} else if (MessageTypeStr == "lbuttondblclk") {
							ConstructMessage.message = WM_LBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "lbuttondown") {
							ConstructMessage.message = WM_LBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "lbuttonup") {
							ConstructMessage.message = WM_LBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "mbuttondblclk") {
							ConstructMessage.message = WM_MBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "mbuttondown") {
							ConstructMessage.message = WM_MBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "mbuttonup") {
							ConstructMessage.message = WM_MBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "mouseactivate") {
							ConstructMessage.message = WM_MOUSEACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "mousehover") {
							ConstructMessage.message = WM_MOUSEHOVER;
							Valid = true;
						} else if (MessageTypeStr == "mousehwheel") {
							ConstructMessage.message = WM_MOUSEHWHEEL;
							Valid = true;
						} else if (MessageTypeStr == "mouseleave") {
							ConstructMessage.message = WM_MOUSELEAVE;
							Valid = true;
						} else if (MessageTypeStr == "mousemove") {
							ConstructMessage.message = WM_MOUSEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "mousewheel") {
							ConstructMessage.message = WM_MOUSEWHEEL;
							Valid = true;
						} else if (MessageTypeStr == "nchittest") {
							ConstructMessage.message = WM_NCHITTEST;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttondblclk") {
							ConstructMessage.message = WM_NCLBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttondown") {
							ConstructMessage.message = WM_NCLBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "nclbuttonup") {
							ConstructMessage.message = WM_NCLBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttondblclk") {
							ConstructMessage.message = WM_NCMBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttondown") {
							ConstructMessage.message = WM_NCMBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncmbuttonup") {
							ConstructMessage.message = WM_NCMBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncmousehover") {
							ConstructMessage.message = WM_NCMOUSEHOVER;
							Valid = true;
						} else if (MessageTypeStr == "ncmouseleave") {
							ConstructMessage.message = WM_NCMOUSELEAVE;
							Valid = true;
						} else if (MessageTypeStr == "ncmousemove") {
							ConstructMessage.message = WM_NCMOUSEMOVE;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttondblclk") {
							ConstructMessage.message = WM_NCRBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttondown") {
							ConstructMessage.message = WM_NCRBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncrbuttonup") {
							ConstructMessage.message = WM_NCRBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttondblclk") {
							ConstructMessage.message = WM_NCXBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttondown") {
							ConstructMessage.message = WM_NCXBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "ncxbuttonup") {
							ConstructMessage.message = WM_NCXBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "rbuttondblclk") {
							ConstructMessage.message = WM_RBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "rbuttondown") {
							ConstructMessage.message = WM_RBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "rbuttonup") {
							ConstructMessage.message = WM_RBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "xbuttondblclk") {
							ConstructMessage.message = WM_XBUTTONDBLCLK;
							Valid = true;
						} else if (MessageTypeStr == "xbuttondown") {
							ConstructMessage.message = WM_XBUTTONDOWN;
							Valid = true;
						} else if (MessageTypeStr == "xbuttonup") {
							ConstructMessage.message = WM_XBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "command") {
							ConstructMessage.message = WM_COMMAND;
							Valid = true;
						} else if (MessageTypeStr == "contextmenu") {
							ConstructMessage.message = WM_CONTEXTMENU;
							Valid = true;
						} else if (MessageTypeStr == "entermenuloop") {
							ConstructMessage.message = WM_ENTERMENULOOP;
							Valid = true;
						} else if (MessageTypeStr == "exitmenuloop") {
							ConstructMessage.message = WM_EXITMENULOOP;
							Valid = true;
						} else if (MessageTypeStr == "gettitlebarinfoex") {
							ConstructMessage.message = WM_GETTITLEBARINFOEX;
							Valid = true;
						} else if (MessageTypeStr == "menucommand") {
							ConstructMessage.message = WM_MENUCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "menudrag") {
							ConstructMessage.message = WM_MENUDRAG;
							Valid = true;
						} else if (MessageTypeStr == "menugetobject") {
							ConstructMessage.message = WM_MENUGETOBJECT;
							Valid = true;
						} else if (MessageTypeStr == "menurbuttonup") {
							ConstructMessage.message = WM_MENURBUTTONUP;
							Valid = true;
						} else if (MessageTypeStr == "nextmenu") {
							ConstructMessage.message = WM_NEXTMENU;
							Valid = true;
						} else if (MessageTypeStr == "uninitmenupopup") {
							ConstructMessage.message = WM_UNINITMENUPOPUP;
							Valid = true;
						} else if (MessageTypeStr == "activate") {
							ConstructMessage.message = WM_ACTIVATE;
							Valid = true;
						} else if (MessageTypeStr == "appcommand") {
							ConstructMessage.message = WM_APPCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "char") {
							ConstructMessage.message = WM_CHAR;
							Valid = true;
						} else if (MessageTypeStr == "deadchar") {
							ConstructMessage.message = WM_DEADCHAR;
							Valid = true;
						} else if (MessageTypeStr == "hotkey") {
							ConstructMessage.message = WM_HOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "keydown") {
							ConstructMessage.message = WM_KEYDOWN;
							Valid = true;
						} else if (MessageTypeStr == "keyup") {
							ConstructMessage.message = WM_KEYUP;
							Valid = true;
						} else if (MessageTypeStr == "killfocus") {
							ConstructMessage.message = WM_KILLFOCUS;
							Valid = true;
						} else if (MessageTypeStr == "setfocus") {
							ConstructMessage.message = WM_SETFOCUS;
							Valid = true;
						} else if (MessageTypeStr == "sysdeadchar") {
							ConstructMessage.message = WM_SYSDEADCHAR;
							Valid = true;
						} else if (MessageTypeStr == "syskeydown") {
							ConstructMessage.message = WM_SYSKEYDOWN;
							Valid = true;
						}
						// MSVC Complained about too many else if's ( compiler limit: blocks nested too deeply )
						if (MessageTypeStr == "syskeyup") {
							ConstructMessage.message = WM_SYSKEYUP;
							Valid = true;
						} else if (MessageTypeStr == "unichar") {
							ConstructMessage.message = WM_UNICHAR;
							Valid = true;
						} else if (MessageTypeStr == "gethotkey") {
							ConstructMessage.message = WM_GETHOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "sethotkey") {
							ConstructMessage.message = WM_SETHOTKEY;
							Valid = true;
						} else if (MessageTypeStr == "initmenupopup") {
							ConstructMessage.message = WM_INITMENUPOPUP;
							Valid = true;
						} else if (MessageTypeStr == "menuchar") {
							ConstructMessage.message = WM_MENUCHAR;
							Valid = true;
						} else if (MessageTypeStr == "menuselect") {
							ConstructMessage.message = WM_MENUSELECT;
							Valid = true;
						} else if (MessageTypeStr == "syschar") {
							ConstructMessage.message = WM_SYSCHAR;
							Valid = true;
						} else if (MessageTypeStr == "syscommand") {
							ConstructMessage.message = WM_SYSCOMMAND;
							Valid = true;
						} else if (MessageTypeStr == "changeuistate") {
							ConstructMessage.message = WM_CHANGEUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "initmenu") {
							ConstructMessage.message = WM_INITMENU;
							Valid = true;
						} else if (MessageTypeStr == "queryuistate") {
							ConstructMessage.message = WM_QUERYUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "updateuistate") {
							ConstructMessage.message = WM_UPDATEUISTATE;
							Valid = true;
						} else if (MessageTypeStr == "canceljournal") {
							ConstructMessage.message = WM_CANCELJOURNAL;
							Valid = true;
						} else if (MessageTypeStr == "queuesync") {
							ConstructMessage.message = WM_QUEUESYNC;
							Valid = true;
						} else if (MessageTypeStr == "dde_ack") {
							ConstructMessage.message = WM_DDE_ACK;
							Valid = true;
						} else if (MessageTypeStr == "dde_advise") {
							ConstructMessage.message = WM_DDE_ADVISE;
							Valid = true;
						} else if (MessageTypeStr == "dde_execute") {
							ConstructMessage.message = WM_DDE_EXECUTE;
							Valid = true;
						} else if (MessageTypeStr == "dde_poke") {
							ConstructMessage.message = WM_DDE_POKE;
							Valid = true;
						} else if (MessageTypeStr == "dde_request") {
							ConstructMessage.message = WM_DDE_REQUEST;
							Valid = true;
						} else if (MessageTypeStr == "dde_terminate") {
							ConstructMessage.message = WM_DDE_TERMINATE;
							Valid = true;
						} else if (MessageTypeStr == "dde_unadvise") {
							ConstructMessage.message = WM_DDE_UNADVISE;
							Valid = true;
						} else if (MessageTypeStr == "dde_initiate") {
							ConstructMessage.message = WM_DDE_INITIATE;
							Valid = true;
						} else if (MessageTypeStr == "ctlcolordlg") {
							ConstructMessage.message = WM_CTLCOLORDLG;
							Valid = true;
						} else if (MessageTypeStr == "enteridle") {
							ConstructMessage.message = WM_ENTERIDLE;
							Valid = true;
						} else if (MessageTypeStr == "getdlgcode") {
							ConstructMessage.message = WM_GETDLGCODE;
							Valid = true;
						} else if (MessageTypeStr == "initdialog") {
							ConstructMessage.message = WM_INITDIALOG;
							Valid = true;
						} else if (MessageTypeStr == "nextdlgctl") {
							ConstructMessage.message = WM_NEXTDLGCTL;
							Valid = true;
						} else if (MessageTypeStr == "devicechange") {
							ConstructMessage.message = WM_DEVICECHANGE;
							Valid = true;
						} else if (MessageTypeStr == "dwmcolorizationcolorchanged") {
							ConstructMessage.message = WM_DWMCOLORIZATIONCOLORCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmcompositionchanged") {
							ConstructMessage.message = WM_DWMCOMPOSITIONCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmncrenderingchanged") {
							ConstructMessage.message = WM_DWMNCRENDERINGCHANGED;
							Valid = true;
						} else if (MessageTypeStr == "dwmsendiconiclivepreviewbitmap") {
							ConstructMessage.message = WM_DWMSENDICONICLIVEPREVIEWBITMAP;
							Valid = true;
						} else if (MessageTypeStr == "dwmsendiconicthumbnail") {
							ConstructMessage.message = WM_DWMSENDICONICTHUMBNAIL;
							Valid = true;
						} else if (MessageTypeStr == "dwmwindowmaximizedchange") {
							ConstructMessage.message = WM_DWMWINDOWMAXIMIZEDCHANGE;
							Valid = true;
						} else if (MessageTypeStr == "copydata") {
							ConstructMessage.message = WM_COPYDATA;
							Valid = true;
						} else if (MessageTypeStr == "setcursor") {
							ConstructMessage.message = WM_SETCURSOR;
							Valid = true;

							// Constants that do not begin with 'WM_'

							// quite a few of those constants

							// so many non 'WM_' constants

						} else if (MessageTypeStr == "askcbformatname") {
							ConstructMessage.message = WM_ASKCBFORMATNAME;
							Valid = true;
						} else if (MessageTypeStr == "changecbchain") {
							ConstructMessage.message = WM_CHANGECBCHAIN;
							Valid = true;
						} else if (MessageTypeStr == "clipboardupdate") {
							ConstructMessage.message = WM_CLIPBOARDUPDATE;
							Valid = true;
						} else if (MessageTypeStr == "destroyclipboard") {
							ConstructMessage.message = WM_DESTROYCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "drawclipboard") {
							ConstructMessage.message = WM_DRAWCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "hscrollclipboard") {
							ConstructMessage.message = WM_HSCROLLCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "paintclipboard") {
							ConstructMessage.message = WM_PAINTCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "renderformat") {
							ConstructMessage.message = WM_RENDERFORMAT;
							Valid = true;
						} else if (MessageTypeStr == "sizeclipboard") {
							ConstructMessage.message = WM_SIZECLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "vscrollclipboard") {
							ConstructMessage.message = WM_VSCROLLCLIPBOARD;
							Valid = true;
						} else if (MessageTypeStr == "clear") {
							ConstructMessage.message = WM_CLEAR;
							Valid = true;
						} else if (MessageTypeStr == "copy") {
							ConstructMessage.message = WM_COPY;
							Valid = true;
						} else if (MessageTypeStr == "cut") {
							ConstructMessage.message = WM_CUT;
							Valid = true;
						} else if (MessageTypeStr == "paste") {
							ConstructMessage.message = WM_PASTE;
							Valid = true;
						} else if (MessageTypeStr == "undo") {
							ConstructMessage.message = WM_UNDO;
							Valid = true;
						}
					}
					

					if (Valid) {
						argi += 3;
					} else {
						return -1;
					}
				}
				try {
					ConstructMessage.wParam = std::stoi(WideParamStr);
				} catch (...) {
					PrintLine("Could not convert "+WideParamStr+" to a wParam Argument");
					return -1;
				}
				try {
					ConstructMessage.lParam = std::stoi(LongParamStr);
				} catch (...) {
					PrintLine("Could not convert "+LongParamStr+" to an lParam Argument");
					return -1;
				}
				MessagesToSend.push_back(ConstructMessage);
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-rs" || arg == "-resize" || arg == "-size") {
			if (argi + 2 < argc) {
				unsigned long long int W = 0, H = 0;

				try {
					W = std::stoi(std::string(argv[argi + 1]));
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a Width");
					return -1;
				}
				try {
					H = std::stoi(argv[argi + 2]);
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 2])+" to a Height");
					return -1;
				}
				argi += 2;
				Width = W;
				Height = H;
				ResizeWindow = true;
				ValidCommand = true;
			} else {
			}
		}

		if (arg == "-h" || arg == "-hidden") {
			ListHidden = true;
			ValidCommand = true;
		}
		if (arg == "-v" || arg == "-verbose") {
			ListUnopened = true;
			ValidCommand = true;
		}
		if (arg == "-p" || arg == "-displaypid") {
			DisplayPID = true;
			ValidCommand = true;
		}
		if (arg == "-i" || arg == "-insensitive") {
			CaseSensitive = false;
			ValidCommand = true;
		}
		if (arg == "-c" || arg == "-close") {
			SendWMClose = true;
			ValidCommand = true;
		}
		if (arg == "-r" || arg == "-reposition" || arg == "-move") {
			RepositionWindow = true;
			ValidCommand = true;

		}
		if (arg == "-f" || arg == "-find") {
			if (argi + 1 < argc) {
				LookFor.push_back(std::string(argv[argi + 1]));
				argi++;
				ValidCommand = true;
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-m" || arg == "-match" || arg == "-pid" || arg == "-matchpid") {
			if (argi + 1 < argc) {
				try {
					MatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					ValidCommand = true;
					return -1;
				}
				ValidCommand = true;
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-nf" || arg == "-negativefind") {
			if (argi + 1 < argc) {
				nLookFor.push_back(std::string(argv[argi + 1]));
				argi++;
				ValidCommand = true;
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-nm" || arg == "-negativematch" || arg == "-negativepid" || arg == "-negativematchpid") {
			if (argi + 1 < argc) {
				try {
					nMatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					ValidCommand = true;
					return -1;
				}
				ValidCommand = true;
			} else {
				PrintLine("Missing Parameters!");
				ValidCommand = true;
				return -1;
			}
		}
		if (arg == "-col" || arg == "-color" || arg == "-colorized" || arg == "-colorify") {
			if (HCON != nullptr) {
				Color = true;
			}
			ValidCommand = true;
		}

		if (!ValidCommand) {
			PrintLine("Unknown Command: " + argO);
			PrintLine("Exiting");
			return -1;
		}
	}
	if (nMatchPID.size() > 0 && MatchPID.size() > 0) {
		PrintLine("please do not match by PID and exclude by PID together");
		return -1;
	}
	if (RepositionWindow) {
		GetCursorPos(&RepositionPoint);
	}
	EnumWindows(EnumWindowsProc, 0);
	PrintLine("");
	bool CausedEffect = (RepositionWindow || ResizeWindow || MessagesToPost.size() > 0 || MessagesToSend.size() > 0 || SendWMClose);
	if (WindowCount > 0) {
		std::string Printout;
		if (CausedEffect) {
			//Printout = "Windows affected: " + std::to_string(WindowCount);
			MaybeColorPrint("Windows affected: ", C_Main);
		} else {
			//Printout = "Windows listed: " + std::to_string(WindowCount);
			MaybeColorPrint("Windows listed: ", C_Main);
		}
		MaybeColorPrint(std::to_string(WindowCount), C_Count);
	} else {
		std::string Printout;
		if (CausedEffect) {
			Printout = "No windows affected";
		} else {
			Printout = "No windows listed";
		}
		MaybeColorPrint(Printout, C_Main);
	}
	if (HCON != nullptr) {
		SetConsoleTextAttribute(HCON, CSBI.wAttributes);
	}
	return 0;
}
