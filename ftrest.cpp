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
#include <netdb.h> 

using namespace std;

#define MAX (1000)

int port;
string command = "";
string remote_path = "";
string local_path = "./";
string serv_name = "";
string type = "folder";
string extension = "null";
string message = "";
string messagefromserv; // Message receiver from serv
string filefromserv; // Subor od serveru
string listItems = ""; // lst items Command
string lengthOfContent = "null"; // Dlzka suboru pre server
string contentType = "text/plain";
string httpError = "null";
int size = 0; // Dlza suboru prijateho od serveru


void errExit(string err) {
	fprintf(stderr, "%s\n", err.c_str());
}

		     
void processArgs (int argc, char *argv[]) {
	// COMMAND ****************************************************
	if (argv[1] != NULL)
		command = argv[1];
	else {
		errExit("Invalid command or missing.");
		exit(1);
	}
	
	// REMOTE-PATH ************************************************
	if (argv[2] != NULL) {
		remote_path = argv[2];
	}
	else {
		errExit("Invalid remote path or missing.");
		exit(1);
	}
	
	// LOCAL-PATH
	if (argv[3] == NULL && command == "put") {
		errExit("Invalid arguments.");
		exit(1);
	}
	if (argv[3] != NULL) {
		local_path = argv[3];
		
		if(local_path[0] != '.')
			local_path.insert (0, 1, '.');
			
		if(local_path[1] != '/')
		local_path.insert(1, 1, '/');
	}
	
	
	if(remote_path.find("http://") == 0)
		remote_path.erase(0,7);
	
	int pos;
		
	// SERVER_NAME
	pos = remote_path.find_first_of(":");
	serv_name = remote_path.substr(0,pos); 
	remote_path.erase(0,pos+1);
	
	// PORT
	pos = remote_path.find_first_of("/");
	port = atoi(remote_path.substr(0, pos).c_str());
	remote_path.erase(0,pos+1);
	if (port > 65535 || port < 0) {
		errExit("Invalid port value.");
		exit(1);
		
	}

	// TYPE & EXTENSION
	for(unsigned int i = 0; i < remote_path.size(); i++) {
		if(remote_path[i] == '.') {
			type = "file";
			extension = remote_path.substr(i+1);
			break;
		}
	}

	if(type == "folder" && (command == "del" || command == "put" || command == "get")) {
		errExit("Not a file.");
		exit(1);
	}

	// FILE CHECK & COMMAND
	if(type == "file" && (command == "lst" || command == "mkd" || command == "rmd")) {
		errExit("Not a directory.");
		exit(1);
	}
}


void createMessage() {
	if(command.compare("put") == 0 || command.compare("mkd") == 0)
		message += "PUT";
	else if (command.compare("del") == 0 || command.compare("rmd") == 0)
		message += "DELETE";
	else if (command.compare("get") == 0 || command.compare("lst") == 0)
		message += "GET";
	else {
		errExit("Invalid arguments.");
		exit(1);
	}

	time_t now = std::time(NULL);
	std::tm * ptm = std::localtime(&now);
	char timeStamp[32];
	std::strftime(timeStamp, 32, "%a, %d %b %Y %H:%M:%S", ptm);  
	
	// Zistujem size suboru pre server
	if (command.compare("put") == 0) {
		ifstream file;
		file.open(local_path, ios::binary);
		if(file) {
			stringstream buffer;
			buffer << file.rdbuf();
			file.close();
			
			buffer.seekg(0, ios::end);
			int size = buffer.tellg();
			lengthOfContent = to_string(size);
		}
		else {
			errExit("Local path error.");
			exit(1);
		}
	}
	
	// MIME typ
	if (type.compare("file") == 0 && command.compare("put") == 0) {
		contentType = "application/octet-stream";
	}
	
	message += string(" /") + remote_path + string("?type=") + type + string(" HTTP/1.1\n");
	message += string("Host: ") + serv_name + string("\n");
	message += string("Date: ") + timeStamp + string(" CET\n");
	message += string("Accept: ") + contentType + string("\n");
	message += string("Accept-Encoding: identity\n");
	message += string("Content-Type: ") + contentType + string("\n");
	message += string("Content-Length: ") + lengthOfContent + string("\n");
}

void processResponse() {
	int pos;
	
	string temp = messagefromserv;
	messagefromserv = "";
	
	
	// Odstranit prve  3 riadky zo spravy od serveru
	for (int i = 0; i < 3; i++) {
		pos = temp.find_first_of("\n");
		messagefromserv += temp.substr(0, pos + 1);
		
		temp.erase(0, pos+1);
	}
	
	
	// Dlzka prijimaneho suboru
	pos = temp.find("\n");
	string getsize = temp.substr(0, pos+1);
	pos = getsize.find(":");
	getsize.erase(0, pos+1);
	size = atoi(getsize.c_str());
	
	
	// Odstranit dalsie 2 riadky zo spravy od serveru
	for (int i = 0; i < 2; i++) {
		pos = temp.find_first_of("\n");
		messagefromserv += temp.substr(0, pos + 1);
		
		temp.erase(0, pos+1);
	}
	
	
	// Http error
	pos = temp.find_first_of("\n");
	httpError = temp.substr(0, pos);
	
	
	// Odstranime posledny riadok
	pos = temp.find_first_of("\n");
	messagefromserv += temp.substr(0, pos + 1);
	temp.erase(0, pos+1);
	
	// Command lst vypis
	if (command.compare("lst") == 0) {
		listItems = temp;
	}
}


int main(int argc, char *argv[]) {
	processArgs(argc, argv);
	createMessage();


	int client_socket;
	struct sockaddr_in serv_addr;
	struct hostent *server;

	
	// Socket()
	if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		errExit("ERROR: socket.");
		exit(1);
	}
	
	memset(&serv_addr, '\0', sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(port);
	
	
	// gethostbyname()
	if ((server = gethostbyname((const char*)serv_name.c_str())) == NULL) {
		errExit("ERROR: gethostbyname.");
		exit(1);
	}
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
	
	
	// Connect()
	if(connect(client_socket, (struct sockaddr *)&serv_addr , sizeof(serv_addr)) < 0) {
		errExit("ERROR: connect");
		exit(1);
	}
	

	// Send()
	if (send(client_socket, message.c_str(), message.size(), 0) < 0) {
		errExit("ERROR: send.");
		exit(1);
	}
	if (command.compare("put") == 0) { // Posielame file serveru
		FILE *file = fopen(local_path.c_str(), "r");
		if(file) {
			char buf[2];
			recv(client_socket, buf, 2, 0);
			string oke = buf;
			if (oke.compare("OK")) {
				send(client_socket, oke.c_str(), 2, 0);
				char toSend[1];
				int ch;
				while( (ch = getc(file)) !=EOF) {
					toSend[0] = ch;
					send(client_socket, toSend ,1, 0);
				}
				fclose(file);
			}
		}
	}
	
	// SPRACOVANIE SPRAVY OD SERVERU ****************************************
	// Recv()
	char buffer[MAX];
	if(recv(client_socket, buffer, MAX, 0) < 0) {
		errExit("ERROR: recv.");
		exit(1);
	}

	messagefromserv = buffer;
	processResponse();
	
	// Save file from serv
	if (size > 0 && command.compare("get") == 0) {
		if(local_path.compare("./") == 0) {
			size_t found = remote_path.find_last_of("/");
			local_path += remote_path.substr(found + 1);
		}
	
		bzero(buffer, MAX);	
		filefromserv = "";		
			
		for(int i = 0; i < size; i++) {
			if (recv(client_socket, buffer, 1, 0) > 0) {
				char ch = buffer[0];
				filefromserv += ch;
				buffer[0] = 0;
			}
		}
		ofstream file(local_path);
		if (file) {
			file << filefromserv;
			file.close();
		}
		else
			errExit("Unknown error.");		
	}	

	
	// Ak je http error od serveru
	if(httpError.compare("null") != 0) {
		errExit(httpError);
		exit(1);
	}	
	
	
	// Ak je lst vypisujeme obsah
	if (command.compare("lst") == 0) {
		cout << listItems;
	}
	// SPRACOVANIE SPRAVY OD SERVERU ****************************************


	// Close()
	if (close(client_socket) < 0) {
		errExit("ERROR: close.");
		exit(1);
	}
	
	return 0;
}
