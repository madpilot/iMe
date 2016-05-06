// Header files
#include <iostream>
#include <fstream>
#include <cstring>
#include <signal.h>
#include <unistd.h>
#include "common.h"
#include "printer.h"
#ifdef USE_GUI
	#include "gui.h"
#endif

using namespace std;


// Global variables
#ifndef USE_GUI
	Printer printer;
#endif


// Function prototypes

/*
Name: Break handler
Purpose: Terminal signal event
*/
void breakHandler(int signal);

/*
Name: Install firmware
Purpose: Installs specified firmware
*/
bool installFirmware(const string &firmwareLocation, const string &serialPort);


// Check if using GUI
#ifdef USE_GUI

	// Implement application
	wxIMPLEMENT_APP(MyApp);
#endif


// Main function
#ifdef USE_GUI
	bool MyApp::OnInit() {
#else
	int main(int argc, char *argv[]) {
#endif
	
	// Attach break handler
	signal(SIGINT, breakHandler);
	
	// Check if using GUI
	#ifdef USE_GUI
	
		// Check if using OS X
		#ifdef OSX
		
			// Enable plainer transition
			wxSystemOptions::SetOption(wxMAC_WINDOW_PLAIN_TRANSITION, 1);
		#endif

		// Create and show window
		MyFrame *frame = new MyFrame("M3D Manager", wxDefaultPosition, wxSize(531, 140), wxDEFAULT_FRAME_STYLE & ~(wxRESIZE_BORDER | wxMAXIMIZE_BOX));
		frame->Center();
		frame->Show(true);
		
		// Return true
		return true;
	
	// Otherwise
	#else
	
		// Display version
		cout << "M3D Manager V" TOSTRING(VERSION) << endl << endl;
	
		// Check if using command line interface
		if(argc > 1) {
	
			// Go through all commands
			for(uint8_t i = 0; i < argc; i++) {
		
				// Check if help is requested
				if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
		
					// Display help
					cout << "Usage: \"M3D Manager\" -d -s -i -m protocol -r firmware.rom serialport" << endl;
					#ifndef OSX
						cout << "-d | --drivers: Install device drivers" << endl;
					#endif
					cout << "-s | --start: Switches printer into firmware mode" << endl;
					cout << "-i | --ime: Installs iMe firmware" << endl;
					cout << "-r | --rom: Installs the provided firmware" << endl;
					cout << "-m | --manual: Allows manually sending commands to the printer using the protocol Repetier or RepRap" << endl;
					cout << "serialport: The printer's serial port or it will automatically find the printer if not specified" << endl << endl;
			
					// Return
					return EXIT_SUCCESS;
				}
			
				// Check if not using OS X
				#ifndef OSX
		
					// Otherwise check if installing drivers
					else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--drivers")) {
			
						// Display message
						cout << "Installing drivers" << endl;
		
						// Check if using Windows
						#ifdef WINDOWS
			
							// Check if creating drivers file failed
							ofstream fout(getTemporaryLocation() + "/M3D.cat", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack drivers
							for(uint64_t i = 0; i < m3D_catSize; i++)
								fout.put(m3D_catData[i]);
							fout.close();
					
							// Check if creating drivers file failed
							fout.open(getTemporaryLocation() + "/M3D.inf", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack drivers
							for(uint64_t i = 0; i < m3D_infSize; i++)
								fout.put(m3D_infData[i]);
							fout.close();
				
							// Check if creating process failed
							TCHAR buffer[MAX_PATH];
							GetWindowsDirectory(buffer, MAX_PATH);
							string path = buffer;
				
							string executablePath;
							ifstream file(path + "\\sysnative\\pnputil.exe", ios::binary);
							executablePath = file.good() ? "sysnative" : "System32";
							file.close();
		
							STARTUPINFO startupInfo;
							SecureZeroMemory(&startupInfo, sizeof(startupInfo));
							startupInfo.cb = sizeof(startupInfo);
						
							PROCESS_INFORMATION processInfo;
							SecureZeroMemory(&processInfo, sizeof(processInfo));
						
							TCHAR command[MAX_PATH];
							_tcscpy(command, (path + "\\" + executablePath + "\\pnputil.exe -i -a \"" + getTemporaryLocation() + "\\M3D.inf\"").c_str());
						
							if(!CreateProcess(NULL, command, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &processInfo)) {

								// Display error
								cout << "Failed to install drivers" << endl;
							
								// Return
								return EXIT_FAILURE;
							}
						
							// Wait until process finishes
							WaitForSingleObject(processInfo.hProcess, INFINITE);
						
							// Check if installing drivers failed
							DWORD exitCode;
							GetExitCodeProcess(processInfo.hProcess, &exitCode);
						
							// Close process and thread handles. 
							CloseHandle(processInfo.hProcess);
							CloseHandle(processInfo.hThread);
						
							if(!exitCode) {
			
								// Display error
								cout << "Failed to install drivers" << endl;
					
								// Return
								return EXIT_FAILURE;
							}
			
							// Display message
							cout << "Drivers successfully installed" << endl;
						#endif
			
						// Otherwise check if using Linux
						#ifdef LINUX
		
							// Check if user is not root
							if(getuid()) {
			
								// Display error
								cout << "Elevated privileges required" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
			
							// Check if creating udev rule file failed
							ofstream fout("/etc/udev/rules.d/90-m3d-local.rules", ios::binary);
							if(fout.fail()) {
					
								// Display error
								cout << "Failed to unpack udev rule" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
					
							// Unpack udev rule
							for(uint64_t i = 0; i < _90_m3d_local_rulesSize; i++)
								fout.put(_90_m3d_local_rulesData[i]);
							fout.close();
			
							// Check if applying udev rule failed
							if(system("/etc/init.d/udev restart")) {
				
								// Display error
								cout << "Failed to install drivers" << endl;
						
								// Return
								return EXIT_FAILURE;
							}
				
							// Display message
							cout << "Drivers successfully installed" << endl;
						#endif
			
						// Return
						return EXIT_SUCCESS;
					}
				#endif
	
				// Otherwise check if a switching printer into firmware mode
				else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--start")) {
		
					// Display message
					cout << "Switching printer into firmware mode" << endl;
		
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Check if connecting to printer failed
					if(!printer.connect(serialPort)) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Check if printer is already in firmware mode
					if(printer.inFirmwareMode())
				
						// Display message
						cout << "Printer is already in firmware mode" << endl;
				
					// Otherwise
					else {
				
						// Put printer into firmware mode
						printer.switchToFirmwareMode();
				
						// Check if printer isn't connected
						if(!printer.isConnected()) {
		
							// Display error
							cout << printer.getStatus() << endl;
						
							// Return
							return EXIT_FAILURE;
						}
				
						// Display message
						cout << "Printer has been successfully switched into firmware mode" << endl;
					}
				
					// Display current serial port
					cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check installing iMe firmware
				else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--ime")) {
		
					// Display message
					cout << "Installing iMe firmware" << endl;
		
					// Set firmware location
					string firmwareLocation = getTemporaryLocation() + "/iMe 1900000001.hex";
		
					// Check if creating iMe firmware ROM failed
					ofstream fout(firmwareLocation, ios::binary);
					if(fout.fail()) {
					
						// Display error
						cout << "Failed to unpack iMe firmware" << endl;
					
						// Return
						return EXIT_FAILURE;
					}
				
					// Unpack iMe ROM
					for(uint64_t i = 0; i < iMe1900000001_hexSize; i++)
						fout.put(iMe1900000001_hexData[i]);
					fout.close();
			
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Install firmware
					if(!installFirmware(firmwareLocation, serialPort))
					
						// Return
						return EXIT_FAILURE;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check if a firmware ROM is provided
				else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--firmwarerom")) {
		
					// Display message
					cout << "Installing firmware" << endl;
	
					// Check if firmware ROM parameter doesn't exist
					if(i >= argc - 1) {
		
						// Display error
						cout << "No firmware ROM provided" << endl;
					
						// Return
						return EXIT_FAILURE;
					}

					// Set firmware location
					string firmwareLocation = static_cast<string>(argv[++i]);
			
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Install firmware
					if(!installFirmware(firmwareLocation, serialPort))
					
						// Return
						return EXIT_FAILURE;
			
					// Return
					return EXIT_SUCCESS;
				}
		
				// Otherwise check if using manual mode
				else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--manual")) {
		
					// Display message
					cout << "Starting manual mode" << endl;
		
					// Check if protocol parameter doesn't exist
					if(i >= argc - 1) {
		
						// Display error
						cout << "No protocol provided" << endl;
					
						// Return
						return EXIT_FAILURE;
					}
			
					// Set protocol
					string protocol = static_cast<string>(argv[++i]);
			
					// Check if protocol is invalid
					if(protocol != "Repetier" && protocol != "RepRap") {
		
						// Display error
						cout << "Invalid protocol" << endl;
					
						// Return
						return EXIT_FAILURE;
					}
		
					// Set serial port
					string serialPort;
					if(i < argc - 1)
						serialPort = argv[argc - 1];
		
					// Check if connecting to printer failed
					if(!printer.connect(serialPort)) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
			
					// Put printer into firmware mode
					printer.switchToFirmwareMode();
				
					// Check if printer isn't connected
					if(!printer.isConnected()) {
		
						// Display error
						cout << printer.getStatus() << endl;
					
						// Return
						return EXIT_FAILURE;
					}
			
					// Display message
					cout << "Enter 'quit' to exit" << endl;
		
					// Loop forever
					while(1) {

						// Get command from user
						cout << "Enter command: ";
						string line;
						getline(cin, line);
					
						// Check if quit is requested
						if(line == "quit")
					
							// Break;
							break;
				
						// Check if failed to send command to the printer
						if(protocol == "Repetier" ? !printer.sendRequestBinary(line) : !printer.sendRequestAscii(line)) {
	
							// Display error
							cout << (printer.isConnected() ? "Sending command failed" : "Printer disconnected") << endl << endl;
						
							// Return
							return EXIT_FAILURE;
						}
				
						// Display command
						cout << "Send: " << line << endl;
				
						// Wait until command receives a response
						for(string response; line != "Q" && line != "M115 S628" && response.substr(0, 2) != "ok" && response.substr(0, 2) != "rs" && response.substr(0, 4) != "skip" && response.substr(0, 5) != "Error";) {
		
							// Get response
							do {
								response = printer.receiveResponseBinary();
							} while(response.empty() && printer.isConnected());
						
							// Check if printer isn't connected
							if(!printer.isConnected()) {
						
								// Display error
								cout << "Printer disconnected" << endl << endl;
							
								// Return
								return EXIT_FAILURE;
							}
				
							// Display response
							if(response != "wait")
								cout << "Receive: " << response << endl;
						}
						cout << endl;
					}
				
					// Return
					return EXIT_SUCCESS;
				}
			}
		
			// Display error
			cout << "Invalid parameters" << endl;
		
			// Return
			return EXIT_FAILURE;
		}
	
		// Otherwise
		else {
	
			// Display error
			cout << "Invalid parameters" << endl;
		
			// Return
			return EXIT_FAILURE;
		}
		
		// Return
		return EXIT_SUCCESS;
	#endif
}


// Supporting function implementation
void breakHandler(int signal) {

	// Terminates the process normally
	exit(EXIT_FAILURE);
}

// Check if not using GUI
#ifndef USE_GUI

	bool installFirmware(const string &firmwareLocation, const string &serialPort) {

		// Check if firmware ROM doesn't exists
		ifstream file(firmwareLocation, ios::binary);
		if(!file.good()) {

			// Display error
			cout << "Firmware ROM doesn't exist" << endl;
		
			// Return false
			return false;
		}

		// Check if firmware ROM name is valid
		uint8_t endOfNumbers = 0;
		if(firmwareLocation.find_last_of('.') != string::npos)
			endOfNumbers = firmwareLocation.find_last_of('.') - 1;
		else
			endOfNumbers = firmwareLocation.length() - 1;

		uint8_t beginningOfNumbers = endOfNumbers - 10;
		for(; beginningOfNumbers && endOfNumbers > beginningOfNumbers && isdigit(firmwareLocation[endOfNumbers]); endOfNumbers--);

		if(endOfNumbers != beginningOfNumbers) {

			// Display error
			cout << "Invalid firmware ROM name" << endl;
		
			// Return false
			return false;
		}

		// Check if connecting to printer failed
		if(!printer.connect(serialPort)) {

			// Display error
			cout << printer.getStatus() << endl;
		
			// Return false
			return false;
		}

		// Check if installing printer's firmware failed
		if(!printer.installFirmware(firmwareLocation.c_str())) {

			// Display error
			cout << "Failed to update firmware" << endl;
		
			// Return false
			return false;
		}

		// Put printer into firmware mode
		printer.switchToFirmwareMode();
	
		// Check if printer isn't connected
		if(!printer.isConnected()) {

			// Display error
			cout << printer.getStatus() << endl;
		
			// Return false
			return false;
		}
	
		// Display message
		cout << "Firmware successfully installed" << endl;
	
		// Display current serial port
		cout << "Current serial port: " << printer.getCurrentSerialPort() << endl;
	
		// Return true
		return true;
	}
#endif