/* 포인터로 만든 연결 리스트(소스) */
#include <stdio.h>
#include <stdlib.h>
#include "Member.h"
#include "01_LinkedList.h"

/*--- 노드를 동적(여기 c파일 내에서만 전역 같이)으로 생성 ---*/
static Node *AllocNode(void)
{
	return calloc(1, sizeof(Node));
}

/*--- n이 가리키는 노드의 각 멤버에 값을 설정, 뒤쪽 포인터 설정 ----*/
static void SetNode(Node *n, const Member *x, const Node *next)
{
	n->data = *x;		/* 데이터 */
	n->next = next;		/* 뒤쪽 포인터 */
}
/*--- 연결 리스트를 사용 전에 초기화 a---*/
void Initialize(List *list)
{
	list->head = NULL;	/* 머리 노드를 초기화 */
	list->crnt = NULL;	/* 선택한 노드(현재 노드)를 초기화 */
}

/*--- compare 함수를 x를 검색 ---*/
Node *search(List *list, const Member *x, int compare(const Member *x, const Member *y))
{
	Node *ptr = list->head;
	while (ptr != NULL) {
			if (compare(&ptr->data, x) == 0) {	/* 키 값이 같은 경우 */
					list->crnt = ptr;
				return ptr;			/* 검색 성공, 해당 포인터 반환 */
			}
		ptr = ptr->next;			/* 다음 노드를 선택 */
	}
	return NULL;					/* 검색 실패, NULL 반환 */
}

/*--- 머리에 노드를 삽입하는 함수 ---*/
void InsertFront(List *list, const Member *x)
{
	Node *ptr = list->head;			/* 기존 헤드값(주소) 가리키는 포인터 생성 */
	list->head = list->crnt = AllocNode();		/* 기존 머리는 새로 할당한 공간 주소를 가리킨다. */
	SetNode(list->head,  x, ptr);		/* 그리고 할당된 공간에 새로운 데이터와 다음 포인터(기존 헤드값)를 설정 */
}

/*--- 꼬리에 노드를 삽입하는 함수 ---*/
void InsertRear(List *list, const Member *x)
{
	if (list->head == NULL)		/* 리스트가 비어 있는 경우 */
		InsertFront(list, x);	/* 머리에 삽입 */
	else {
		Node *ptr = list->head;		/* 헤드 주소 받아서 ptr에 넣기 */
		while (ptr->next != NULL)	/* next가 NULL값(꼬리 노드) 찾을 때까지 */
			ptr = ptr->next;
		ptr->next = list->crnt = AllocNode();	/* 꼬리 노드 next에 새로 할당된 공간 주소 */
		SetNode(ptr->next, x, NULL);	/* 그 주소에 데이터 설정 next주소는 NULL */
	}
}

/*--- 머리 노드를 삭제하는 함수 ---*/
void RemoveFront(List *list)
{
	if (list->head != NULL) {				/* 리스트 머리 노드가 있으면 */
		Node *ptr = list->head->next;		/* 두 번째 노드 주소를 담는 포인터 생성 */
		free(list->head);					/* 머리 노드 기존 주소로 할당된 공간 반환*/
		list->head = list->crnt = ptr;		/* 머리 노드 포인터를 두 번째 노드 주소 값을 가진 포인터로 설정*/
	}
}

/*--- 꼬리 노드를 삭제하는 함수 --*/
void RemoveRear(List *list)
{
	if (list->head != NULL) {
		if ((list->head)->next == NULL)	/* 리스트에 노드가 1개인 경우 */
			RemoveFront(list);			/* 꼬리 노드가 곧 머리 노드, 머리 노드 삭제 */
		else {
			Node *ptr = list->head;		/* ptr은 head부터 꼬리로 찾아갈 준비 */
			Node *pre = NULL;			/* pre는 일단 선언 */
			while (ptr->next != NULL) {		/* 꼬리 찾기 전까지 */
				pre = ptr;					/* pre는 ptr을 받으면 ptr 이전 노드를 가리킴 */
				ptr = ptr->next;			/* ptr은 다음 노드 주소를 받는다. */
			}
			
			pre->next = NULL;	/* 기존 꼬리의 이전 노드를 가리키는 pre의 next를 NULL로 설정해 새로운 꼬리 노드로 만들기 */
			free(ptr);			/* ptr이 가리키는 주소에 할당된 공간 반환; 기존 꼬리 노드 삭제 */
			list->crnt = pre;	/* 현재 포인터를 새로운 꼬리를 가리킨다? 응? 이건 왜? */
		}
	}
}

/*--- 선택한 노드를 삭제하는 함수 ---*/
void RemoveCurrent(List *list)
{
	if (list->head != NULL) {
		if (list->crnt == list->head)	/* 머리 노드를 선택한 상태라면 */
			RemoveFront(list);			/* 머리 노드를 삭제 */
		else {
			Node *ptr = list->head;			/* ptr은 head를 가리키고 crnt 찾아갈 준비 */
			while (ptr->next != list->crnt) /* crnt 만나기 전까지 */
				ptr = ptr->next;			/* ptr은 next로 교체 되며 이동 */
			ptr->next = list->crnt->next;	/* ptr next가 crnt와 같아지면, crnt next를 받는다. */
			free(list->crnt);				/* crnt가 가리키는 주소 공간을 반환 */
			list->crnt = ptr;				/* crnt가 가리키는 주소를 ptr로 갱신 */
		}
	}
}

/*--- 모든 노드를 삭제하는 함수 ---*/
void Clear(List *list)
{
	while (list->head != NULL)		/* 리스트에 노드가 존재하면 */
		RemoveFront(list);			/* head == NULL일 때까지 머리 노드를 계속 삭제 */
	list->crnt = NULL;				/* 노드가 없으므로 crnt를 NULL로 갱신(중요) */
}

/*--- 선택한 노드의 데이터를 출력 ---*/
void PrintCurrent(const List *list)
{
	if (list->crnt == NULL)
		printf("선택한 노드가 없습니다.");
	else
		PrintMember(&list->crnt->data);	/* data 출력하는 함수가 Member.c에 있는 듯? */
}
/*--- 선택한 노드의 데이터를 출력(줄 바꿈 문자 포함) ---*/
void PrintLnCurrent(const List *list)
{
	PrintCurrent(list);
	putchar('\n');
}

/*--- 모든 노드의 데이터를 리스트 순으로 출력하는 함수 ---*/
void Print(const List *list)
{
	if (list->head == NULL)
		puts("노드가 없습니다.");
	else {
		Node *ptr = list->head;
		puts("모두 보기");
		while (ptr != NULL) {
			PrintLnMember(&ptr->data);		/* &를 붙이면 현재 구조체를 가리키는 것과 같다. */
			ptr = ptr->next;		
		}
	}
}
/*--- 연결 리스트를 종료하는 함수 ---*/
void Terminate(List *list)
{
	Clear(list);		/* 모든 노드를 삭제와 같다. */
}