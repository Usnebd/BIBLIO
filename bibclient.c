#include<sys/socket.h>
#include<sys/un.h>
#include<sys/types.h>
#define UNIX_PATH_MAX 80
#define SOCKNAME "welcomeSocket"

int main(int argc, char* argv[]){
    struct sockaddr_un sa;
    sa.sun_family=AF_UNIX;
    strncpy(sa.sun_path, SOCKNAME,UNIX_PATH_MAX);
    int serverSock=socket(AF_UNIX,SOCK_STREAM,0);
    while(connect(serverSock,(struct sockaddr*)&sa),sizeof(sa) == -1){
        if(errno == ENOENT){
            sleep(1);
        }else exit(EXIT_FAILURE);
    }
}