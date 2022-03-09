/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void serve_dynamic(int fd, char *filename, char *cgiargs);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
                 char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 듣기 소켓 열기
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    // 무한 서버 루프 - 연결 요청 접수
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
    port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    // 트랜잭션 처리
    doit(connfd);   // line:netp:tiny:doit

    // 자신의 연결 닫기
    Close(connfd);  // line:netp:tiny:close
  }
}

// 트랜잭션 하나 처리
void doit(int fd)
{
  int is_static;
  struct stat sbuf;
  char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
  char filename[MAXLINE], cgiargs[MAXLINE];
  rio_t rio;

  /* Read request line and headers */
  // 요청라인 읽고 분석
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);
  sscanf(buf, "%s %s %s", method, uri, version);
  // 클라이언트가 GET 아닌 다른 메소드를 요청하면, 에러메시지
  if (strcasecmp(method, "GET")) {
    clienterror(fd, method, "501", "Not implemented",
              "Tiny does not implement this method");
    return;
  }
  // 다른 메소드 헤더 무시
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  // 정적 동적 컨텐츠인지 플래그 설정
  is_static = parse_uri(uri, filename, cgiargs);
  // 파일이 디스크 상에 없으면 즉시 클라이언트에 에러메시지
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", 
              "Tiny couldn't find this file");
    return;
  }

  // 정적 컨텐츠라면
  if (is_static) { /* Serve static content */
    // 보통 파일이고 읽기 권한이 있는가 검증
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    // 해당하면 클라이언트에게 제공
    serve_static(fd, filename, sbuf.st_size);
  }
  // 동적 컨텐츠라면
  else { /* Serve dynamic content */
    // 보통 파일이고 읽기 권한이 있는가 검증
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", 
                  "Tiny couldn't run the CGI program");
      return;
    }
    // 해당하면 클라이언트에게 제공
    serve_dynamic(fd, filename, cgiargs);
  }
}

// 에러 처리
void clienterror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Tiny Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Tiny Web server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}

void read_requesthdrs(rio_t *rp)
{
  char buf[MAXLINE];

  Rio_readlineb(rp, buf, MAXLINE);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
  }
  return;
}

// URI 분석
// 정적 컨텐츠은 현재 디렉토리, 실행파일의 홈 디렉토리는 "/cgi-bin"
// 동적 컨텐츠는 스트링 cgi-bin을 포함하는 모든 URI, 기본 파일 이름은 ./home.html
// parse_uri함수는 URI을 파일이름과 옵션으로 CGI 인자 스트링을 분석
int parse_uri(char *uri, char *filename, char *cgiargs)
{
  char *ptr;

  // Static content

  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");                // cgiargs(인자스트링) 안에 "" 넣어서 비우기
    strcpy(filename, ".");
    strcat(filename, uri);
    if (uri[strlen(uri)-1] == '/')
      strcat(filename, "home.html");
    return 1;
  }

  // Dynamic content
  else {
    ptr = index(uri, '?');
    if (ptr) {
      strcpy(cgiargs, ptr+1);
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");      
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

// 정적 컨텐츠 클라이언트에게 제공
void serve_static(int fd, char *filename, int filesize)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  // 파일 타입 검사
  get_filetype(filename, filetype);

  // 응답라인 응답헤더 보내기
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  /* Send response body to client */
  // 요청파일을 오픈해서 식별자 srcfd에 가져온다.
  srcfd = Open(filename, O_RDONLY, 0);
  // mmap함수는 요청한 파일(식별자srcfd)을 가상메모리 영역으로 첫번째 filesize바이트를 매핑한다.  
  srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // srcfd 더이상 필요 없어 닫는다.
  Close(srcfd);
  // 실제파일을 클라이언트에게 전송한다.
  // Rio_writen함수는 주소 srcp에서 시작하는 filesize바이트를 클라이언트 연결 식별자로 복사
  Rio_writen(fd, srcp, filesize);
  // 매핑된 가상메모리 주소를 반환
  Munmap(srcp, filesize);
}

// get_filetype - Derive file type from filename
void get_filetype(char *filename, char *filetype)
{
  if (strstr(filename, ".html"))
    strcpy(filetype, "text/html");
  else if (strstr(filename, ".gif"))
    strcpy(filetype, "image/gif");
  else if (strstr(filename, ".png"))
    strcpy(filetype, "image/png");
  else if (strstr(filename, ".jpg"))
    strcpy(filetype, "image/jpeg");
  // 동영상 mp4 파일 설정
  else if (strstr(filename, ".mp4"))
    strcpy(filetype, "video/mp4");
  else 
    strcpy(filetype, "text/plain");
}

// 동적 컨텐츠 클라이언트에게 제공
void serve_dynamic(int fd, char *filename, char *cgiargs)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // Child
  // 새로운 자식 프로세스를 fork한다.
  if (Fork() == 0) {
    /* Real server would set all CGI vars here */
    // cgiargs 환경변수들로 초기화
    setenv("QUERY)STRING", cgiargs, 1);
    // Redirect stdout to client
    // 자식은 자신의 연결파일 식별자로 재지정
    Dup2(fd, STDOUT_FILENO);
    // Run CGI program
    Execve(filename, emptylist, environ);
  }
  // Parent waits for and reaps child
  Wait(NULL);
}