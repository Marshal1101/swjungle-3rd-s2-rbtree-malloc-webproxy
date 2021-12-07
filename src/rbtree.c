#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>

rbtree *new_rbtree(void) {
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  node_t *nil = (node_t *)calloc(1, sizeof(node_t));
  nil->color = RBTREE_BLACK;
  nil->left = nil;
  nil->right = nil;
  p->nil = nil;
  p->root = nil;
  p->root->parent = nil;
  return p;
}

void delete_node(rbtree *t, node_t *x) {
  if (x == t->nil)
    return;
  delete_node(t, x->left);
  delete_node(t, x->right);
  free(x);
}

void delete_rbtree(rbtree *t) {
  // TODO: reclaim the tree nodes's memory
  delete_node(t, t->root);
  free(t->nil);
  free(t);
}

node_t *right_rotate(rbtree *t, node_t *z) {
// z로 우회전 할 때에는 왼쪽 자식 노드를 피벗
// 피벗의 오른쪽 자식 노드를 부모노드의 왼쪽 자식으로 연결
  node_t *pvt;
  pvt = z->left;
  z->left = pvt->right;
  if (pvt->right != t->nil)
    pvt->right->parent = z;
  pvt->parent = z->parent;
  // case 1 z가 루트
  if (z->parent == t->nil)
    t->root = pvt;
  // case 2 z가 부모의 왼쪽 자식
  else if (z == z->parent->left)
    z->parent->left = pvt;
  // case 3 z가 부모의 오른쪽 자식
  else
    z->parent->right = pvt;
  // z 부모로 pvt, pvt 오른쪽 자식에 z 설정 
  z->parent = pvt;
  pvt->right = z;
  return t->root;
}

node_t *left_rotate(rbtree *t, node_t *z) {
// z로 좌회전 할 때에는 오른쪽 자식 노드를 피벗
// 피벗의 왼쪽 자식 노드를 부모노드의 오른쪽 자식으로 연결
  node_t *pvt;
  pvt = z->right;
  z->right = pvt->left;
  if (pvt->left != t->nil)
    pvt->left->parent = z;
  pvt->parent = z->parent;
  if (z->parent == t->nil)
    t->root = pvt;
  else if (z == z->parent->left)
    z->parent->left = pvt;
  else
    z->parent->right = pvt;
  z->parent = pvt;
  pvt->left = z;
  return t->root;
}

node_t *rbtree_insert_fixup(rbtree *t, node_t *z) {
  node_t *y;
  while (z->parent->color == RBTREE_RED) {
    // case left : 삽입노드의 부모가 조부모의 왼쪽 자식
    if (z->parent == z->parent->parent->left) {
      y = z->parent->parent->right;
      // case 1 : 삼촌 y가 적색인 경우
      // 부모노드와 삼촌노드를 검은색, 조부노드를 빨간색 변경
      if (y->color == RBTREE_RED) {
        z->parent->color = RBTREE_BLACK;
        y->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        // CLRS says: z.p.p를 새로운 z로 놓는다
        z = z->parent->parent;
      }
      else {
        // case 2 : 삼촌 y가 흑색, z가 부모의 오른쪽 자식
        // 부모노드를 왼쪽으로 회전시켜서 문제를 3번으로 변경
        if (z == z->parent->right) {
            // z에 z 부모를 담아서 왼쪽 회전시켜면,
            // z는 기존 z의 자식으로 변경되어서 case 3 이동
            z = z->parent;
            left_rotate(t, z);
        }
        // case 3 : 삼촌 y가 흑색, z가 부모의 왼쪽 자식
        // 부모노드를 검은색, 조부노드를 빨간색으로 칠한다.
        // 조부노드를 기준으로 오른쪽 회전을 한다.
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        right_rotate(t, z->parent->parent);
      }
    }
    // case right : 삽입노드의 부모가 조부모의 오른쪽 자식
    // case left에서 right와 left를 바꾼 경우와 같다.
    else {
      y = z->parent->parent->left;
      if (y->color == RBTREE_RED) {
        z->parent->color = RBTREE_BLACK;
        y->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        z = z->parent->parent;
      }
      else {
        if (z == z->parent->left) {
          z = z->parent;
          right_rotate(t, z);
        }
      z->parent->color = RBTREE_BLACK;
      z->parent->parent->color = RBTREE_RED;
      left_rotate(t, z->parent->parent);
      }
    }
  }
  t->root->color = RBTREE_BLACK;  
  return t->root;
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  // TODO: implement insert
  // 새로운 노드 z를 생성해 key값을 받아 이진 트리 삽입
  // x는 루트부터 시작할 포인터이고 z를 위치를 찾을 것이다.
  // y는 nil에서 시작할 z의 부모가 될 노드이다.
  node_t *z = (node_t *)calloc(1, sizeof(node_t));
  node_t *x = t->root;
  node_t *y = t->nil;
  // 삽입할 key와 포인터x의 key를 비교하며,
  // 삽입key가 더 작으면 왼쪽, 더 크면 오른쪽으로 보낸다.
  while (x != t->nil) {
    y = x;
    if (key < x->key)
      x = x->left;
    else
      x = x->right;
  }
  // while 탈출: 더 이상 내려갈 곳이 없다.
  // 그 자리가 새로운 노드의 자리
  // y는 z의 부모로 설정, y가 nil이면 z는 루트라는 뜻
  // z가 y보다 작으면 y->left, 반대면 y->right
  z->parent = y;
  if (y == t->nil)
    t->root = z;
  else if (key < y->key)
    y->left = z;
  else
    y->right = z;
  z->left = t->nil;
  z->right = t-> nil;
  z->key = key;
  z->color = RBTREE_RED;
  rbtree_insert_fixup(t, z);
  return z;
}

node_t *rbtree_find(const rbtree *t, const key_t key) {
  node_t *ptr = t->root;
  while (ptr != t->nil) {
    if (key < ptr->key)
      ptr = ptr->left;
    else if (key > ptr->key)
      ptr = ptr->right;
    else
      return ptr;
  }
  return NULL;
}

node_t *rbtree_min(const rbtree *t) {
  node_t *ptr = t->root;
  while (ptr->left != t->nil) {
    ptr = ptr->left;
  }
  return ptr;
}

node_t *rbtree_max(const rbtree *t) {
  node_t *ptr = t->root;
  while (ptr->right != t->nil) {
    ptr = ptr->right;
  }
  return ptr;
}

// 추가된 함수
void rbtree_transplant(rbtree *t, node_t *u, node_t *v) {
  if (u->parent == t->nil)
    t->root = v;
  else if (u == u->parent->left)
    u->parent->left = v;
  else u->parent->right = v;
  v->parent = u->parent;
}

// 추가된 함수
void rbtree_erase_fixup(rbtree *t, node_t *x) {
// *x 삭제 대상, w는 x의 형제를 가리키는 포인터
  node_t *w;
  while (x!= t->root && x->color == RBTREE_BLACK) {
    if (x == x->parent->left) {
      w = x->parent->left;
      if (w->color == RBTREE_RED) {
        w->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        left_rotate(t, x->parent);
        w = x->parent->right;
      }
      if (w->left->color == RBTREE_BLACK && w->right->color == RBTREE_BLACK) {
        w->color = RBTREE_RED;
        x = x->parent;
      }
      else {
        if (w->right->color == RBTREE_BLACK) {
          w->left->color = RBTREE_BLACK;
          w->color = RBTREE_RED;
          right_rotate(t, w);
          w = x->parent->right;
        }
        w->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        w->right->color = RBTREE_BLACK;
        left_rotate(t, x->parent);
        x = t->root;
      }
    }
    else {
            w = x->parent->right;
      if (w->color == RBTREE_RED) {
        w->color = RBTREE_BLACK;
        x->parent->color = RBTREE_RED;
        right_rotate(t, x->parent);
        w = x->parent->left;
      }
      if (w->right->color == RBTREE_BLACK && w->left->color == RBTREE_BLACK) {
        w->color = RBTREE_RED;
        x = x->parent;
      }
      else {
        if (w->left->color == RBTREE_BLACK) {
          w->right->color = RBTREE_BLACK;
          w->color = RBTREE_RED;
          left_rotate(t, w);
          w = x->parent->left;
        }
        w->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        w->left->color = RBTREE_BLACK;
        right_rotate(t, x->parent);
        x = t->root;
      }
    }
  }
x->color = RBTREE_BLACK;
}

int rbtree_erase(rbtree *t, node_t *p) {
  // 여기서 p는 CLRS에서 나오는 z ; 삭제 대상
  node_t *y = p;
  color_t y_origin_color = y->color;
  node_t *x;
  // case1 삭제대상의 왼쪽 자식이 nil
  if (p->left == t->nil) {
    x = p->right;
    rbtree_transplant(t, p, p->right);
  }
  // case2 삭제대상의 오른쪽 자식이 nil
  else if (p->right == t->nil) {
    x = p->left;
    rbtree_transplant(t, p, p->left);
  } 
  // case3 삭제대상의 자식이 모두 nil이 아님
  else {
    // 대체 삭제자 y의 등장
    // while: tree-minimum; p292 최소 키 찾아가기
    while (p->right->left != t->nil){
      p->right = p->right->left;
    }
    //y는 p->right를 루트로 하는 서브트리에서 최소 키를 갖는 노드
    y = p->right;
    y_origin_color = y->color;
    x = y->right;
    if (y->parent == p)
      x->parent = y;
    else {
      rbtree_transplant(t, y, y->right);
      y->right = p->right;
      y->right->parent = y;
    }
    rbtree_transplant(t, p, y);
    y->left = p->left;
    y->left->parent = y;
    y->color = p->color;
  }
  if (y_origin_color == RBTREE_BLACK)
    rbtree_erase_fixup(t, x);

  free(y);
  return 0;
}

// 추가된 함수
int inorder_tree_work(const rbtree *t, const node_t *x, key_t *arr, int i) {
  if (x->left != t->nil) 
    i = inorder_tree_work(t, x->left, arr, i); 
  arr[i] = x->key;
  i = i+1;
  if (x->right != t->nil) 
    i = inorder_tree_work(t, x->right, arr, i);
  return i;
  }


int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {

  inorder_tree_work(t, t->root, arr, 0);
  
  return 0;
}



// int main(){
//   rbtree *t = new_rbtree();
//   rbtree_insert(t, 10);
//   rbtree_insert(t, 5);
//   rbtree_insert(t, 8);
//   rbtree_insert(t, 34);
//   rbtree_insert(t, 67);
//   rbtree_insert(t, 23);
//   rbtree_insert(t, 156);
//   rbtree_insert(t, 24);
//   rbtree_insert(t, 2);
//   rbtree_insert(t, 12);
//   int n =10;
//   key_t *res = calloc(n, sizeof(key_t));
//   rbtree_to_array(t, res, n);
//   printf("[");
//   for (int i = 0; i < n; i++) {
//   printf("%d: ", res[i]);
//   node_t *node = rbtree_find(t, res[i]);
//   if(node->color == RBTREE_RED){
//     printf("RED,");
//   }else{
//     printf("BLACK,");
//   }
//   }
//   printf("]\n");
//   node_t *min = rbtree_min(t);
//   printf("최소 %d", min->key);
//   node_t *max = rbtree_max(t);
//   printf("최대 %d", min->key);
//   return 0;
// }