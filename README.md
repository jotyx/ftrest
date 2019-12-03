## NAME
ftrest-client - Send a command to server  
ftrestd-server - Server executing operations requested by client  

## SYNOPSIS
`./ftrestd [-r ROOT-FOLDER] [-p PORT]`  
`./ftrest COMMAND REMOTE-PATH [LOCAL-PATH]`  

## DESCRIPTION
Client-server application with BSD sockets. Communication is  
based on HTTP and RESTful API (PUT, GET, DELETE).

## OPTIONS
**ftrestd:**

`-r ROOT-FOLDER` is root directory for server work (default `./`)

`-p PORT` is port number of listening server (deafult `6677`)

**ftrest:**

`COMMAND`:
- `del` delete file on REMOTE-PATH on server
- `get` copy file from REMOTE-PATH to client actual folder
- `put` copy file from LOCAL-PATH to server REMOTE-PATH folder
- `lst` client lists content of folder on server
- `mkd` make folder on REMOTE-PATH on server
- `rmd` remove folder on REMOTE-PATH on server

`REMOTE-PATH` is path to file/folder on server

`LOCAL-PATH` (optional) is path to file/folder on client

## EXAMPLES
* Launch a server on port 1234. It will be responding to all next examples:  
`./ftrestd -p 1234`  

* Create a folder named *Documents* on server at localhost:  
`./ftrest mkd http://localhost:1234/Documents`  

* Remove a previously created folder:  
`./ftrest rmd http://localhost:1234/Documents`  

* Put a file *list.txt* from local path to the server root directory:  
`./ftrest put http://localhost:1234/list.txt /list.txt`  

* Delete previously uploaded file:  
`./ftrest del http://localhost:1234/list.txt`  

* Download a file *image.png* from server to local directory:  
`./ftrest get http://localhost:1234/Images/image.png`  

* List all items on specified path in server:  
`./ftrest lst http://localhost:1234/Documents/`  

## DIAGNOSTICS
- *"Not a directory."*  when command is lst or rmd, but client requires file
- *"Directory not found."*  when command is lst or rmd, but folder doesn't exists
- *"Directory not empty."*  when command is rmdir, but folder is not empty
- *"Already exists."*  when command is mkd or put, but file/folder already exists
- *"Not a file."*  when command is del or get, but client requires folder
- *"File not found."*  when command is del or get, but file doesn't exists
- *"Unknown error."*  for other errors
