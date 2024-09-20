// Copyright Ethan M. Hardt 2024
// Licensed under the MIT License
// See License.txt for details


#include <Windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>

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

// Options:
bool ListHidden			= false;
bool ListUnopened		= false;
bool DisplayPID			= false;
bool CaseSensitive		= true;
bool RepositionWindow	= false;
bool SendWMClose		= false;
bool Color				= false;



TextColor C_Main	= TextColor::LightGrey;
TextColor C_Title	= TextColor::Green;
TextColor C_Path	= TextColor::Blue;
TextColor C_PID		= TextColor::Red;
TextColor C_Extra	= TextColor::Magenta;

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
	bool DoRepos = RepositionWindow;
	if (!IsWindowVisible(Window)) {
		if (!ListHidden) {
			return TRUE; // specify -h to list hidden windows.
		}
		DoRepos = false;
		// Prevents making hidden windows visible.
		// Programs love making hidden windows (can't blame them, especially when making a modern OpenGL context...)
	}
	char Ch[256];
	if (GetWindowTextA(Window, Ch, 256) > 0) {
		std::string WindowTitle = Ch;
		MaybeColorPrint(WindowTitle, C_Title);

		if (WindowTitle == "Program Manager") {
			DoRepos = false;
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
					MaybeColorPrint(" belongs to a process we query the name of", C_Main);
					//PrintLine(WindowTitle + " Belongs to a process we query the name of" + AdditionalInfo);
				}
			} else {
				if (ListUnopened) {
					MaybeColorPrint(" belongs to a process we couldn't open", C_Main);
					//PrintLine(WindowTitle + " Belongs to a process we couldn't open" + AdditionalInfo);
				} else {
					DoRepos = false;
				}
			}
			if (DisplayPID) {
				MaybeColorPrint(" with PID ", C_Main);
				MaybeColorPrint("#"+std::to_string(ProcID), C_PID);
				//AdditionalInfo += " with PID of " + std::to_string(ProcID);
			}
		} else {
			if (ListUnopened) {
				MaybeColorPrint(" belongs to an unknown process", C_Main);
				//PrintLine(WindowTitle + " Belongs to an unknown process");
			} else {
				DoRepos = false;
			}
		}
		if (SendWMClose) {
			if (PIDValid) {
				SendMessage(Window, WM_CLOSE, 0, 0);
				DoRepos = false;
				// Repositioning and Closing should not happen together..
			}
		}
		EndLine();
	} else {
		DoRepos = false;
		// Prevents moving Classic start menu.
	}
	if (DoRepos) {
		SetWindowPos(Window, HWND_TOP, RepositionPoint.x, RepositionPoint.y, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);
		// Hopefully this helps with someone who moved their window offscreen.
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
	} else if (str == "dark red" || str == "darkred") {
		Col = TextColor::DarkRed;
	} else if (str == "dark magenta" || str == "darkmagenta") {
		Col = TextColor::DarkMagenta;
	} else if (str == "dark blue" || str == "darkblue") {
		Col = TextColor::DarkBlue;
	} else if (str == "dark cyan" || str == "darkcyan") {
		Col = TextColor::DarkCyan;
	} else if (str == "dark green" || str == "darkgreen") {
		Col = TextColor::DarkGreen;
	} else if (str == "dark yellow" || str == "darkyellow") {
		Col = TextColor::DarkYellow;
	} else if (str == "black") {
		Col = TextColor::Black;
	} else if (str == "dark gray" || str == "dark grey" || str == "dark grae" || str == "darkgray" || str == "darkgrey" || str == "darkgrae") {
		Col = TextColor::DarkGrey;
	} else if (str == "light gray" || str == "light grey" || str == "light grae" || str == "lightgray" || str == "lightgrey" || str == "lightgrae" || str == "gray" || str == "grey" || str == "grae") {
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

				DataC = 1024;
				// Color_Extra is a catch-all.
				if (RegGetValue(RegKey, nullptr, "Color_Extra", RRF_RT_REG_SZ, &Type, &Char, &DataC) == ERROR_SUCCESS) {
					Col_Title = Char;
					Col_Path = Char;
					Col_PID = Char;
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
				C_Main	= StrToCol(Col_Main);
				C_Title	= StrToCol(Col_Title);
				C_Path	= StrToCol(Col_Path);
				C_PID	= StrToCol(Col_PID);
				C_Extra	= StrToCol(Col_Extra);
			}
		}
	}


	for (int argi = 0; argi < argc; argi++) {
		std::string arg = argv[argi];
		if (arg.size() > 0) {
			if (arg.at(0) == '/') {
				arg.at(0) = '-';
			}
		}
		for (uint64_t A = 0; A < arg.size(); A++) {
			arg.at(A) = Lower(arg.at(A));
		}



		if (arg == "-?" || arg == "-help") {
			PrintLine("Window Wrangler Help:");
			PrintLine("Usage: WndWrangle.exe [switch arguments]");
			PrintLine("");
			PrintLine("for all listed arguments below, you can use either / or -");
			PrintLine("-about					Shows the about screen");
			PrintLine("-license				Displays the license");
			PrintLine("-? or -help				Shows this Help Screen");
			PrintLine("-h or -hidden				Lists windows that are hidden");
			PrintLine("-v or -verbose				Lists windows of unknown processes (Verbose)");
			PrintLine("-p or -displaypid			Prints the PID");
			PrintLine("-i or -insensitive			Search is insensitive to case");
			PrintLine("-c or -close				Send WM_CLOSE to the listed windows. (filter with -f, -nf, and/or -m)");
			PrintLine("-r or -reposition			Repositions the visible listed windows to the mouse (except Program Manager)");
			PrintLine("-f or -find	[title]			Find a window by title");
			PrintLine("-nf or -negativefind	[title]		Filter window out by title");
			PrintLine("-m or -match	[PID]			Match a window by program ID");
			PrintLine("-nm or -negativematch	[PID]		Unmatch a window by program ID");
			PrintLine("-col or -color 		Unmatch a window by program ID");
			return 0;
		}
		if (arg == "-about") {
			PrintLine("Window Wrangler");
			PrintLine("by Miles (Ethan M. Hardt)");
			PrintLine("version 1.1.2");
			PrintLine("");
			PrintLine("Licensed under MIT");
			PrintLine("See license.txt for the license");
			PrintLine("or use -license");
			PrintLine("(Please do not change the");
			PrintLine("license for downstream distributions)");
			PrintLine("");
			PrintLine("Personal Note:");
			PrintLine("Please do not call me by my legal name,");
			PrintLine("unless you are legally required to,");
			PrintLine("or I have met you in person.");
			PrintLine("It is included for legal reasons.");
			return 0;
		}
		if (arg == "-license") {
			PrintLine("MIT License");
			PrintLine("");
			PrintLine("Copyright (c) 2024 Ethan Miles Hardt");
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
			return 0;
		}
		if (arg == "-h" || arg == "-hidden") {
			ListHidden = true;
		}
		if (arg == "-v" || arg == "-verbose") {
			ListUnopened = true;
		}
		if (arg == "-p" || arg == "-displaypid") {
			DisplayPID = true;
		}
		if (arg == "-i" || arg == "-insensitive") {
			CaseSensitive = false;
		}
		if (arg == "-c" || arg == "-close") {
			SendWMClose = true;
		}
		if (arg == "-r" || arg == "-reposition" || arg == "-move") {
			RepositionWindow = true;
		}
		if (arg == "-f" || arg == "-find") {
			if (argi + 1 < argc) {
				LookFor.push_back(std::string(argv[argi + 1]));
				argi++;
			}
		}
		if (arg == "-m" || arg == "-match" || arg == "-pid" || arg == "-matchpid") {
			if (argi + 1 < argc) {
				try {
					MatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					return -1;
				}
			}
		}
		if (arg == "-nf" || arg == "-negativefind") {
			if (argi + 1 < argc) {
				nLookFor.push_back(std::string(argv[argi + 1]));
				argi++;
			}
		}
		if (arg == "-nm" || arg == "-negativematch" || arg == "-negativepid" || arg == "-negativematchpid") {
			if (argi + 1 < argc) {
				try {
					nMatchPID.push_back(std::stoi(std::string(argv[argi + 1])));
					argi++;
				} catch (...) {
					PrintLine("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					return -1;
				}
			}
		}
		if (arg == "-col" || arg == "-color" || arg == "-colorized" || arg == "-colorify") {
			if (HCON != nullptr) {
				Color = true;
			}
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
	if (HCON != nullptr) {
		SetConsoleTextAttribute(HCON, CSBI.wAttributes);
	}
	return 0;
}
