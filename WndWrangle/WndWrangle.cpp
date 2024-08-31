// Copyright Ethan M. Hardt 2024
// Licensed under the MIT License
// See License.txt for details


#include <Windows.h>
#include <psapi.h>
#include <iostream>
#include <string>
#include <vector>

void Print(std::string Str) {
	std::cout << Str << std::endl << std::flush;
}

// Options:
bool ListHidden			= false;
bool ListUnopened		= false;
bool DisplayPID			= false;
bool CaseSensitive		= true;
bool RepositionWindow	= false;
bool SendWMClose		= false;

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
			std::string AdditionalInfo;
			if (DisplayPID) {
				AdditionalInfo += " with PID of " + std::to_string(ProcID);
			}

			HANDLE H = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, ProcID);
			if (H != nullptr) {
				DWORD ExSize = 4096;
				if (QueryFullProcessImageNameA(H, 0, Ch2, &ExSize) != 0) {
					std::string ProcessName = Ch2;
					Print(WindowTitle + " Belongs to " + ProcessName + AdditionalInfo);
					CloseHandle(H);
				} else {
					Print(WindowTitle + " Belongs to a process we query the name of" + AdditionalInfo);
				}
			} else {
				if (ListUnopened) {
					Print(WindowTitle + " Belongs to a process we couldn't open" + AdditionalInfo);
				} else {
					DoRepos = false;
				}
			}
		} else {
			if (ListUnopened) {
				Print(WindowTitle + " Belongs to an unknown process");
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


int main(int argc, char* argv[]) {
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
			Print("Window Wrangler Help:");
			Print("Usage: WndWrangle.exe [switch arguments]");
			Print("");
			Print("for all listed arguments below, you can use either / or -");
			Print("-about					Shows the about screen");
			Print("-license				Displays the license");
			Print("-? or -help				Shows this Help Screen");
			Print("-h or -hidden				Lists windows that are hidden");
			Print("-v or -verbose				Lists windows of unknown processes (Verbose)");
			Print("-p or -displaypid			Prints the PID");
			Print("-i or -insensitive			Search is insensitive to case");
			Print("-c or -close				Send WM_CLOSE to the listed windows. (filter with -f, -nf, and/or -m)");
			Print("-r or -reposition			Repositions the visible listed windows to the mouse (except Program Manager)");
			Print("-f or -find	[title]			Find a window by title");
			Print("-nf or -negativefind	[title]		Filter window out by title");
			Print("-m or -match	[PID]			Match a window by program ID");
			Print("-nm or -negativematch	[PID]		Unmatch a window by program ID");
			return 0;
		}
		if (arg == "-about") {
			Print("Window Wrangler");
			Print("by Miles (Ethan M. Hardt)");
			Print("version 1.1.1");
			Print("");
			Print("Licensed under MIT");
			Print("See license.txt for the license");
			Print("or use -license");
			Print("(Please do not change the");
			Print("license for downstream distributions)");
			Print("");
			Print("Personal Note:");
			Print("Please do not call me by my legal name,");
			Print("unless you are legally required to,");
			Print("or I have met you in person.");
			Print("It is included for legal reasons.");
			return 0;
		}
		if (arg == "-license") {
			Print("MIT License");
			Print("");
			Print("Copyright (c) 2024 Ethan Miles Hardt");
			Print("");
			Print("Permission is hereby granted, free of charge, to any person obtaining a copy");
			Print("of this software and associated documentation files (the \"Software\"), to deal");
			Print("in the Software without restriction, including without limitation the rights");
			Print("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell");
			Print("copies of the Software, and to permit persons to whom the Software is");
			Print("furnished to do so, subject to the following conditions:");
			Print("");
			Print("The above copyright notice and this permission notice shall be included in all");
			Print("copies or substantial portions of the Software.");
			Print("");
			Print("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR");
			Print("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,");
			Print("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE");
			Print("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER");
			Print("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,");
			Print("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE");
			Print("SOFTWARE.");
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
					Print("Could not convert "+std::string(argv[argi + 1])+" to a PID");
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
					Print("Could not convert "+std::string(argv[argi + 1])+" to a PID");
					return -1;
				}
			}
		}
	}
	if (nMatchPID.size() > 0 && MatchPID.size() > 0) {
		Print("please do not match by PID and exclude by PID together");
		return -1;
	}
	if (RepositionWindow) {
		GetCursorPos(&RepositionPoint);
	}
	EnumWindows(EnumWindowsProc, 0);
	return 0;
}
