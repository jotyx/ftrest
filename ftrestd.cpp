#include <iostream>
#include <sstream>
#include <fstream> 
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <ctime>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX (1000)

using namespace std;

int port = 6677;
string root = "/";
string command = "";
string type = "";
string path = ""; // Cela cesta aj s root
string contentFromClient = "null"; // File from client
string cli_request = ""; // Sprava od klienta
string contentType = "text/plain"; // Type of content for response
string serv_response = ""; // Odpoved pre klienta
string endOfMessageForClient = ""; // File or LST
string lengthOfContent = "null"; // Length of content for client;
string responseCode = "not defined"; // HTTP Code of operation
int size = 0; // size of client file
string returnError = "null";


void errExit(string err) {
	returnError = err;
}

void processArgs (int argc, char *argv[]) {
	int argument;
	while ((argument = getopt(argc, argv, "r:p:")) != -1) {
		switch (argument) {
			case 'p':
				port = atoi(optarg);
				break;
			case 'r':
				root = optarg;
				break;
			default:
				fprintf(stderr, "Invalid arguments.\n");
				exit(1);
		}
	}
	
	if (port > 65535 || port < 0) {
		fprintf(stderr, "Invalid port number.\n");
		exit(1);
	}
	
	// Ošetrenie root argumentu
	if (root.compare("/") != 0) {
		if(root.back() != '/') {
			root += string("/");
		}
		if(root[0] != '.') {
			root.insert(0, 1, '.');
		}	
	}
	
	// Root neexistuje check
	struct stat buf;	
	if(stat(root.c_str(), &buf) != 0) {
		fprintf(stderr, "Root path invalid.\n");
		exit(1);
	}
}


void parseMessage() {
	string temp = cli_request;
	string trimmed = "";
	int pos;
	
	// Trim message for nice look
	for (int i = 0; i < 7; i++) {
		pos = temp.find_first_of("\n");
		trimmed += temp.substr(0, pos + 1);
		
		temp.erase(0, pos+1);
	}


	if(cli_request[0] == 'D') {
		command = string("DELETE");
		cli_request.erase(0,8);
	}	
	else if(cli_request[0] == 'G') {
		command = string("GET");
		cli_request.erase(0,5);
	}
	else if(cli_request[0] == 'P') {
		command = string("PUT");
		cli_request.erase(0,5);
	}

	// First line of message
	string line;
	pos = cli_request.find("\n");
	line = cli_request.substr(0,pos); 
	pos = line.find("HTTP/1.1");
	line = line.substr(0, pos-1);

	
	if (line.back() == 'r') // Zistenie typu
		type = string("folder");
	else
		type = string("file");
	
	for(unsigned int i = 0; i < line.length(); i++) {
		if (line[i] == '?') {
			path = root + line.substr(0, i);
			break;
		}
	}
	if (path[0] != '.')
		path.insert (0, 1, '.');
	
	
	// Remove next 6 lines from message by client
	for (int i = 0; i < 6; i++) {
		pos = cli_request.find_first_of("\n");
		cli_request.erase(0, pos+1);
	}
	
	// Get size of file from client
	pos = cli_request.find(":");
	cli_request.erase(0,pos+1);
	size = atoi(cli_request.c_str());
		
	cli_request = trimmed;
}


void createResponse() {
	time_t now = std::time(NULL);
	std::tm * ptm = std::localtime(&now);
	char timeStamp[32];
	std::strftime(timeStamp, 32, "%a, %d %b %Y %H:%M:%S", ptm); 
	
	// Content type
	if (type.compare("file") == 0 && command.compare("GET") == 0) {
		contentType = "application/octet-stream";
	}

	serv_response += string("HTTP/1.1 ") + responseCode + string("\n");
	serv_response += string("Date: ") + timeStamp + string(" CET\n");
	serv_response += string("Content-Type: ") + contentType + string("\n");
	serv_response += string("Content-Length: ") + lengthOfContent + string("\n");
	serv_response += string("Content-Encoding: ") + string("identity\n");
	serv_response += returnError + string("\n");
	serv_response += endOfMessageForClient;
}


void takeAction() {
	struct stat buf;
	
	if(stat(path.c_str(), &buf) == 0 && S_ISREG(buf.st_mode)) { // Je to subor & existuje
	
		if(type.compare("file") == 0) { // Typ FILE******
		
			if(command.compare("PUT") == 0) {
			
				errExit("Already exists.");
				responseCode = "400 Bad Request";
			}
			else if(command.compare("DELETE") == 0) {
			
				if(remove(path.c_str()) < 0) {
					errExit("Unknown error.");
				}
				else
					responseCode = "200 OK";
			}
			else if(command.compare("GET") == 0) {
			
				ifstream file(path);
				if(file) {
					stringstream buf;
					buf << file.rdbuf();
					
					buf.seekg(0, ios::end);
					int size = buf.tellg();
					lengthOfContent = to_string(size);
					responseCode = "200 OK";
				}
				else {
					errExit("Unknown error.");
				}
			}
		}
		
		else if (type.compare("folder") == 0) { // Typ FOLDER****
			
			if (command.compare("PUT") == 0) {
				responseCode = "404 Not Found";
				errExit("Already exists.");
			}
			else { // DEL, GET
				responseCode = "400 Bad Request";
				errExit("Not a directory");
			}
		}

	}
	else if(stat(path.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode)) { // Je to directory & existuje
		
		if(type.compare("file") == 0) { // Typ FILE******
		
			if(command.compare("PUT") == 0) {
				responseCode = "400 Bad Request";
				errExit("Already exists.");
			}
			else { // DEL, GET
				responseCode = "400 Bad Request";
				errExit("Not a file.");
			}
		}
		
		else if (type.compare("folder") == 0) { // Typ FOLDER****
			DIR *directory;
			struct dirent *d;
			
			if (command.compare("PUT") == 0) {
				responseCode = "400 Bad Request";
				errExit("Already exists.");
			}
			else if (command.compare("DELETE") == 0) { // rmd
				
				if ( (directory = opendir(path.c_str()) ) != NULL) {
					closedir(directory);
					responseCode = "200 OK";

					int status = rmdir(path.c_str());
					if (status != 0) {
						errExit("Directory not empty.");
						responseCode = "400 Bad Request";
					}
				}
				else
					errExit("Unknown error.");
			}
			else if (command.compare("GET") == 0) { // lst
			
				if ( (directory = opendir(path.c_str())) != NULL) {
					while((d = readdir(directory)) != NULL) {
						if(strcmp(d->d_name, ".") == 0 || strcmp(d->d_name, "..") == 0)
							continue;
					
						endOfMessageForClient += d->d_name + string("\n");
					}
					closedir(directory);
					responseCode = "200 OK";
				}
				else {		
					errExit("Unknown error.");
				}	
			}
		}
	}
	else if(stat(path.c_str(), &buf) != 0) { // Neexistuje

		if(type.compare("file") == 0) {
		
			if(command.compare("PUT") == 0) {
				
				int pos = path.find_last_of("/");
				string dir = path.substr(0, pos);
				
				if (stat(dir.c_str(), &buf) == 0 || S_ISDIR(buf.st_mode)) {
				
					ofstream file;
					file.open(path, ios::binary);
					if (file) {
						file << contentFromClient;
						file.close();
						responseCode = "200 OK";
					}
					else
						errExit("Unknown error.");
				}
				else {
					errExit("Unknown error.");
				}
			}
			
			else { // DEL, GET
				responseCode = "404 Not Found";
				errExit("File not found.");
			}
		}
		
		else if (type.compare("folder") == 0) {
		
			if(command.compare("PUT") == 0) { // mkd

				if(path.back() == '/')
					path.pop_back();
					
				string temp = path;
				
				size_t found = temp.find_last_of("/");
				temp = temp.substr(found);
				
				// Verify if path is valid
				string verifyIfExist = path.substr(0, found);
				if(! (stat(verifyIfExist.c_str(), &buf) == 0 && S_ISDIR(buf.st_mode)) ) {
					errExit("Directory not found.");
					responseCode = "404 Not Found";
				}
				else if ( !(mkdir(path.c_str(), 0700) != 0 || stat(temp.c_str(), &buf) != 0 ||
				    !S_ISDIR(buf.st_mode)) ) {
					errExit("Unknown error.");   	
				}
				else {
					responseCode = "200 OK";
				}
			}
			
			else if (command.compare("GET") == 0 || command.compare("DELETE") == 0) {
				responseCode = "404 Not Found";
				errExit("Directory not found.");
			}
		}
	}
	else { // Je to err
		errExit("Unknown error.");
		responseCode = "404 Not Found";
	}
}


int main(int argc, char *argv[]) {
	int server_socket, newsockfd;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	int one = 1;
	
	processArgs(argc, argv);
	

	// Socket()
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		fprintf(stderr, "ERROR: socket.\n");
		return -1;
	}
	
	//REUSEADDR
	if(setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
		fprintf(stderr, "ERROR: reuseaddr.\n");
		return -1;
	}
	
	memset(&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	
	
	// Bind()
	if(bind(server_socket, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		fprintf(stderr, "ERROR: bind.\n");
		return -1;
	}
	
	
	// Listen()
	if ((listen(server_socket, 5)) < 0) {
		fprintf(stderr, "ERROR: listen.\n");
		return -1;
	}
	
	
	// Accept()
	while(1) { // Server online
	int cli_len = sizeof(struct sockaddr_in);
	if ((newsockfd = accept(server_socket, (struct sockaddr *)&cli_addr, (socklen_t*)&cli_len)) < 0) {
		fprintf(stderr, "ERROR: accept.\n");
		return -1;
	}	
	else {
		char buffer[MAX];
		bzero(buffer, MAX);
		
		
		// Recv()
		if (recv(newsockfd, buffer, MAX, 0) < 0) {
			fprintf(stderr, "ERROR: recv.\n");
			return -1;
		}
		cli_request = buffer;

		
		// SPRACOVANIE RESPONSU *********
		parseMessage();
		
		// Prijimame file od klienta
		if (size > 0 && command.compare("PUT") == 0) {
			string oke = "OK";
			send(newsockfd, oke.c_str(), 2, 0);
		
			bzero(buffer, MAX);	
			contentFromClient = "";		
			
			recv(newsockfd, buffer, 2, 0);
			oke = buffer;
			if(oke.compare("OK") == 0) {
				for(int i = 0; i < size; i++) {
					if (recv(newsockfd, buffer, 1, 0) > 0) {
						char ch = buffer[0];
						contentFromClient += ch;
						buffer[0] = 0;
					}
				}
			}
		}
		
		takeAction();
		createResponse();
		// SPRACOVANIE RESPONSU *********
		
			
		// Send()
		if (send(newsockfd, serv_response.c_str(), MAX, 0) < 0) {
			fprintf(stderr, "ERROR: send.\n");
		}
		// Posielame file klientovi
		if (command.compare("GET") == 0 && type.compare("file") == 0) { 
			FILE *file = fopen(path.c_str(), "r");
			if(file) {
				char toSend[1];
				int ch;
				while( (ch = getc(file)) !=EOF) {
					toSend[0] = ch;
					send(newsockfd, toSend ,1, 0);
				}
			fclose(file);
			}
		}

		
		// Reset global variables
		command = "";
		type = "";
		path = ""; 
		contentFromClient = "";
		cli_request = "";
		endOfMessageForClient = "";
		responseCode = "";	
		serv_response = "";
		size = 0;
		contentType = "null";
		returnError = "null";
	
	
		// Close() client
		if (close(newsockfd) < 0) {
			fprintf(stderr, "ERROR: close client sock.\n");
			return -1;
		}
	}
	} // end while(1)
	
	
	// Close() server
	if (close(server_socket) < 0) {
		fprintf(stderr, "ERROR: close server sock.\n");
		return -1;
	}
	
	return 0;
}
