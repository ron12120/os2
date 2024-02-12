#define PORT 80
#define ADDR "127.0.0.1"

typedef struct get {
    char get_str[4];
    char *remote_path;
    char crlf[3];
} get, *pget;


typedef struct post {
    char post_str[5];
    char *remote_path;
    char crlf_1[3];
    char *file_base64;
    char crlf_2[3];
    char crlf_3[3];
} post, *ppost;


typedef struct std_res {
    char ok[7];
    char crlf_1[3];
    char *file_base64;
    char crlf_2[3];
    char crlf_3[3];
} std_res, *pstd_res;


typedef struct err_res {
    char code[5];
    char *err_msg;
    char crlf_1[3];
    char crlf_3[3];
} err_res, *perr_res;
