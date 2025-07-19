How to Run files
"gcc tftp_server.c tftp.c -o tftp_server" and "gcc tftp_client.c tftp.c -o tftp_client"
it will create executable files "tftp_server" and "tftp_client"
then in one terminal we have to run "./tftp_server" and another terminal we have run "./tftp_client"
int client side we have to give commands like 
connect ip address(actual your Ip address");
put file(actual file name)
get file(actual file name)
help (it will display  the contents)
tftp> help
Commands:
connect <ip> [port]
get <filename>
put <filename>
bye/quit
help
tftp> 






