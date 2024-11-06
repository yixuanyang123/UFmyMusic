# UFmyMusic
## Overview
This project, UFmyMusic, is designed to synchronize music file directories across multiple machines through a networked application. By ensuring all machines have identical song collections, it addresses discrepancies in music libraries. The application supports multiple client connections to a centralized server handling various file operations.
## Prerequisites
Ensure you have SSH access to connect to the school's remote server. SSH (Secure Shell) is necessary for securely logging into and executing commands on rain.cise.ufl.edu from a remote location. This access allows you to upload, manage, and synchronize project files directly on the server environment.
## Project Structure
- `server.cpp` - Implements the server logic.
- `client.cpp` - Implements the client interface and commands.
- `makefile` - Contains build instructions for the entire project.
## Required Directory Structure
- `music/`: This directory is on the server side and contains all the music files that are available for synchronization with the client.
- `local_music/`: This directory is on the client side and holds the music files that have been synchronized from the server or are available locally for synchronization.
## Compiling and Running Instructions
1. Connect to the UF remote server using SSH, for example:
   ```bash
   ssh rain.cise.ufl.edu -l yixuanyang
2. Uploading the project folder to the UF server, for example:
   ```bash
   scp -r ./Desktop/project_2 yixuanyang@rain.cise.ufl.edu:.
3. Navigate to the project directory and compile the source code:
   ```bash
   cd project
   make
4. Start the server application:
   ```bash
   ./changeServer
 - The server will start and wait for the connection from clients.
5. In a new terminal session, connect again to the UF server and start the client application in the project_2 folder:
   ```bash
   ./nameChanger
- Upon running, the client will prompt you with a simple text-based interface to choose from the available commands (LIST, DIFF, PULL, LEAVE).
6. Note: If a `.DS_Store` file is automatically generated after running the LIST command, please remove this file by using `rm .DS_Store` before proceeding with subsequent steps.
## User Interface
The client interface operates via command-line prompts. Here's how to use each feature:
- **LIST**: Use the LIST command to retrieve and display a list of all files currently stored in the music directory on the server. This command provides an overview of what is available for synchronization.
- **DIFF**: The DIFF command compares the contents of your local_music directory with the music directory on the server. It lists the files that are present on the server but missing from your local directory, indicating the files you need to download.
- **PULL**: After identifying the differences with the DIFF command, use PULL to download the missing files from the music directory on the server to your local_music directory. This command ensures your local collection is synchronized with the server’s collection.
- **LEAVE**: Type LEAVE to properly close your session and disconnect from the server. This command ensures that the connection is closed safely, preventing potential data loss or corruption.
