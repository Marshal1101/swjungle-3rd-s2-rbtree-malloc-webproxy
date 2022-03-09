/*
 * tiny.c - A simple, iterative HTTP/1.0 Web server that uses the
 *     GET method to serve static and dynamic content.
 */
#include "csapp.h"

void doit(int fd);
void read_requesthdrs(rio_t *rp);
int parse_uri(char *uri, char *filename, char *cgiargs);
// void serve_static(int fd, char *filename, int filesize);
void serve_static(int fd, char *filename, int filesize, char *method);
void get_filetype(char *filename, char *filetype);
// void serve_dynamic(int fd, char *filename, char *cgiargs);
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method);
void clienterror(int fd, char *cause, char *errnum,
    char *shortmsg, char *longmsg);

int main(int argc, char **argv)
{
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;

  /* Check command line args */
  // 서버 실행에 입력된 인자의 개수가 2개가 아니라면 표준 오류 출력 ex) ./tiny 8000
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  // 듣기 소켓 열기, argv[1] = 포트번호
  listenfd = Open_listenfd(argv[1]);

  while (1) {
    clientlen = sizeof(clientaddr);
    // 무한 서버 루프 - 연결 요청 접수 connfd 생성
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);  // line:netp:tiny:accept

    // 성공을 알리는 함수
    Getnameinfo((SA *)&clientaddr, clientlen, hostname, MAXLINE,
    port, MAXLINE, 0);
    printf("Accepted connection from (%s, %s)\n", hostname, port);

    // 트랜잭션 처리(connfd가 이곳에 들어간다.)
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
  // 요청라인 읽고 분석, rio_t타입 버퍼 rio에 연결
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  printf("Request headers:\n");
  printf("%s", buf);      // 최초 요청라인 : GET / HTTP/1.1
  // buf 에 있는 내용을, method, uri, version이라는 문자열에 저장한다.
  sscanf(buf, "%s %s %s", method, uri, version);      
  // 클라이언트가 GET 아닌 다른 메소드를 요청하면, 에러메시지
  // strcasecmp함수는 method, "GET" 두 문자를 비교해서 있으면 0, 없으면 1
  
// if (strcasecmp(method, "GET")) {                     //line:netp:doit:beginrequesterr
  if (strcasecmp(method, "GET") && strcasecmp(method,  "HEAD")) { // 같으면 0반환이라 if문 안들어감 1은 true라 에러실행 // Tiny는 GET메소드만 지원. 만약 다른 메소드(like POST)를 요청하면. strcasecmp : 문자열비교.
    clienterror(fd, method, "501", "Not implemented", "Tiny does not implement this method"); // 에러메시지 보내고, main루틴으로 돌아온다
    return; // 그 후 연결 닫고 다음 요청 기다림. 그렇지 않으면 읽어들이고
  }
  // 요청라인을 제외한 요청헤더들을 읽어서 출력
  read_requesthdrs(&rio);

  /* Parse URI from GET request */
  // 정적 동적 컨텐츠인지 플래그 설정
  // uri를 통해 filename과 cgiargs을 채워 넣는다. 리턴값 정적이면 1, 동적이면 0 -> is_static
  is_static = parse_uri(uri, filename, cgiargs);
  // stat(1, 2) 1의 인자 정보들을 2에 넣어서 성공하면 0, 실패하면 -1
  // 파일이 디스크 상에 없으면 (-1) 즉시 클라이언트에 에러메시지
  if (stat(filename, &sbuf) < 0) {
    clienterror(fd, filename, "404", "Not found", 
              "Tiny couldn't find this file");
    return;
  }

  // is_static이 1이면 (정적 컨텐츠라면)
  if (is_static) { /* Serve static content */
    // 보통 파일이고 읽기 권한이 있는가 검증
    //S_ISREG: sbuf.st_mode를 받아서 정규 파일인지 판별
    //S_IRUSR: sbuf.st_mode를 받아서 사용자 읽기 가능인지 판단
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden",
                  "Tiny couldn't read the file");
      return;
    }
    // 해당하면 클라이언트에게 제공
    // serve_static(fd, filename, sbuf.st_size);        //line:netp:doit:servestatic
    serve_static(fd, filename, sbuf.st_size, method);        //line:netp:doit:servestatic
  }
  // is_static이 0이면 (동적 컨텐츠라면)
  else { /* Serve dynamic content */
    if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {
      clienterror(fd, filename, "403", "Forbidden", 
                  "Tiny couldn't run the CGI program");
      return;
    }
    // 해당하면 클라이언트에게 제공
    // serve_dynamic(fd, filename, cgiargs);            //line:netp:doit:servedynamic
    serve_dynamic(fd, filename, cgiargs, method);            //line:netp:doit:servedynamic
  }
}

// 에러 처리
void clienterror(int fd, char *cause, char *errnum,
                char *shortmsg, char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  // html body를 sprintf로 작성하는 법
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
  // 여러 요청 중에 첫번째는 무시
  Rio_readlineb(rp, buf, MAXLINE);
  // 다음부터 while문으로 받아서 출력
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
  // strstr(str1, str2): str1에서 str2가 시작하는 시점을 찾는다, 
  // 이때 string2가 없는 경우 NULL return
  //strcpy(arr, str): arr에 str을 복사
  if (!strstr(uri, "cgi-bin")) {
    strcpy(cgiargs, "");                // cgiargs(인자스트링) 안에 "" 넣어서 비우기
    strcpy(filename, ".");              // .
    // strcat(filename, uri) filename 뒤에 uri를 붙인다.
    strcat(filename, uri);              // .uri
    if (uri[strlen(uri)-1] == '/')      // 마지막 문자열이 / 이라면
      strcat(filename, "home.html");    // home.html 붙인다.
    return 1;
  }

  // Dynamic content: 소스 약속 cgi-bin 문자가 있으면 -> 동적콘텐츠
  else {
    // ?의 인덱스를 찾는다. 없으며 NULL 반환
    ptr = index(uri, '?');
    // ptr, 즉 인덱스가 있으면
    if (ptr) {
      // ? 다음 문자열을 cgiargs에 복사
      strcpy(cgiargs, ptr+1);
      // \0 은 끝을 표시
      *ptr = '\0';
    }
    else
      strcpy(cgiargs, "");      
    // filename 에 . 을 붙이고 uri 붙인다. -> 결과 ex) ./cgi-bin/adder
    strcpy(filename, ".");
    strcat(filename, uri);
    return 0;
  }
}

// 정적 컨텐츠 클라이언트에게 제공
// fd는 connfd
void serve_static(int fd, char *filename, int filesize, char *method)
{
  int srcfd;
  char *srcp, filetype[MAXLINE], buf[MAXBUF];

  /* Send response headers to client */
  // filename의 확장자를 검사해서 filetype에 담는다.
  get_filetype(filename, filetype);

  // 응답라인 응답헤더 보내기
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  sprintf(buf, "%sServer: Tiny Web Server\r\n", buf);
  // tiny 서버 구조상 한 번 while 시행하면 닫히고 새로운 connfd만든다.
  sprintf(buf, "%sConnection: close\r\n", buf);
  sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
  sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
  // Rio_writen 안에 새로운 버퍼를 생성해서 그곳에서 fd로 보낸다.  
  Rio_writen(fd, buf, strlen(buf));
  printf("Response headers:\n");
  printf("%s", buf);

  // HEAD면 리턴
  if (strcasecmp(method, "HEAD") == 0)
      return;
  /* Send response body to client */
  // 11.9 adjusted
  // 요청파일을 오픈해서 식별자 srcfd에 가져온다.
  srcfd = Open(filename, O_RDONLY, 0);
  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
  // /*파일이나 디바이스를 응용 프로그램의 주소 공간 메모리에 대응시킨다.
  //   1인자 => 시작 포인터 주소 (아래의 예제 참조)
  //   2인자 => 파일이나 주소공간의 메모리 크기
  //   3인자 => PROT 설정 (읽기, 쓰기, 접근권한, 실행)
  //   4인자 => flags는 다른 프로세스와 공유할지 안할지를 결정한다.
  //   5인자 => fd는 쓰거나 읽기용으로 열린 fd값을 넣어준다.
  //   6인자 => offset은 0으로 하던지 알아서 조절한다.*/

  // srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  //line:netp:servestatic:mmap
  // Mmap과 Mummap 메모리를 할당하고 메모리공간에 데이터를 넣고, 반환할 때 데이터를 시스템에 되돌린다.
  srcp = (char*)Malloc(filesize);
  Rio_readn(srcfd, srcp, filesize);
  Close(srcfd);                           //line:netp:servestatic:close
  Rio_writen(fd, srcp, filesize);         //line:netp:servestatic:write
  // Munmap(srcp, filesize);              Mamp이 할당 malloc이라면 Munmap은 반환 free
  free(srcp);
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
void serve_dynamic(int fd, char *filename, char *cgiargs, char *method)
{
  char buf[MAXLINE], *emptylist[] = { NULL };

  /* Return first part of HTTP response */
  sprintf(buf, "HTTP/1.0 200 OK\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Server: Tiny Web Server\r\n");
  Rio_writen(fd, buf, strlen(buf));

  // Child
  // 새로운 자식 프로세스를 fork(생성)한다.
  // 호출한 프로세스는 부모 프로세스가 됙, 새롭게 생성된 프로세스는 자식 프로세스가 된다.
  // 이때 부모 프로세스는 자식의 PID(Process ID)를 넘겨받고, 자식 프로세스는 새로 0을 받는다.
  // fork 값을 기준으로 분기 처리를 하면 각 프로세스의 메모리 공간이 달라진다.
  if (Fork() == 0) {
    /* Real server would set all CGI vars here */
    // cgiargs 환경변수들로 초기화, 리눅스 쿼리스트링에 cgiargs를 강제(1)로 넣는다. 0은 기존에 있으면 패스
    setenv("QUERY_STRING", cgiargs, 1);
    setenv("REQUEST_METHOD", method, 1);
    // Redirect stdout to client
    // 자식은 자신의 연결파일 식별자(1)로 재지정
    Dup2(fd, STDOUT_FILENO);
    // Run CGI program
    // 프로그램 실행함수로, 첫번째 인자는 프로그램의 경로, 세번째 인자는 환경 변수목록을 의미
    Execve(filename, emptylist, environ);
  }
  // Parent waits for and reaps child
  Wait(NULL);
}