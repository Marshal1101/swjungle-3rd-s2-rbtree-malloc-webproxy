#include "rbtree.h"

#include <stdlib.h>

rbtree *new_rbtree(void) {
  rbtree *p
   = (rbtree *)calloc(1, sizeof(rbtree));
  // TODO: initialize struct if needed
  p->nil = (node_t *)calloc(1, sizeof(node_t));
  p->nil->color = RBTREE_BLACK;
  p->root = p->nil;
  return p;
}

void delete_node(rbtree *t, node_t *x) {
  if (x == t->nil)
    return;
  delete_node(t, x->left);
  delete_node(t, x->right);
  free(x);
  x = NULL;
}


void delete_rbtree(rbtree *t) {
  // TODO: reclaim the tree nodes's memory
  
  delete_node(t, t->root);
  free(t->nil);
  free(t);
  t->nil = NULL;
  t = NULL;  
}

// 추가된 함수
void left_rotate(rbtree *t, node_t *x) {
// 트리 받고, x를 받는다. x는 FIXUP에서 이미 부모로 설정되고 들어온 z의 부모이다.
// CLRS 책과 통일하기 위해 y, x, z 매개변수 설정
  node_t *y = x->right;
  x->right = y->left;
  if (y->left != t->nil) {
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == t->nil) {
    t->root = y;
  }
  else if (x == x->parent->left) {
    x->parent->left = y;
  }
  else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
}

// 추가된 함수
void right_rotate(rbtree *t, node_t *y) {
// 트리 받고, x를 받는다. x는 FIXUP에서 이미 부모로 설정되고 들어온 z의 부모이다.
// CLRS 책과 통일하기 위해 y, x, z 매개변수 설정
// right_rotate는 left_rotate 코드와 대칭
  node_t *x = y->left;
  y->left = x->right;
  if (x->right != t->nil) {
    x->right->parent = y;
  }
  x->parent = y->parent;
  if (y->parent == t->nil) {
    t->root = x;
  }
  else if (y == y->parent->left) {
    y->parent->left = x;
  }
  else {
    y->parent->right = x;
  }
  x->right = y;
  y->parent = x;
}

// 추가된 함수
node_t *rbtree_insert_fixup(rbtree *t, node_t *z) {
  while (z->parent->color == RBTREE_RED) {
  // while: z 초기 대상은 새로운 노드로서 레드;
  // 즉 현재 노드가 부모와 레드 중복인 경우이다.
    if (z->parent == z->parent->parent->left) {
      // z의 부모가 왼쪽 자식이면
      node_t *y = z->parent->parent->right;
      // y 는 오른쪽 자식; 엉클로 설정
      if (y->color == RBTREE_RED) {
      // 경우1 삼촌도 레드
        z->parent->color = RBTREE_BLACK;
        y->color = RBTREE_BLACK;
        z->parent->parent = RBTREE_RED;
        z = z->parent->parent;
        // 다음 체크대상은 기존 z의 조부모
      }
      else {
        if (z == z->parent->right) {
      // 경우2 z가 부모의 오른쪽 자식이면(값이 더 크면)
        z = z->parent;
        // 새로운 기준은 기존 z의 부모로 설정하고
        void left_rotate(rbtree *t, node_t *z);
        // 레프트회전을 하면 부모로 설정된 z가
        // 이제 자식이 되고 원래 자식이 부모가 된다(z->parent)
        }
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        void right_rotate(rbtree *t, node_t *z);
      }
    }
    else {
      // else는 if에서 left와 right 만 바꾼 경우 
      // z의 부모가 왼쪽 자식이 아니면
      node_t *y = z->parent->parent->left;
      // y 는 왼쪽 자식; 엉클로 설정
      if (y->color == RBTREE_RED) {
      // 경우1 삼촌도 레드
        z->parent->color = RBTREE_BLACK;
        y->color = RBTREE_BLACK;
        z->parent->parent = RBTREE_RED;
        z = z->parent->parent;
        // 다음 체크대상은 기존 z의 조부모
      }
      else {
        if (z == z->parent->left) {
        // 경우2 z가 부모의 왼쪽 자식이면(값이 더 작으면)
        z = z->parent;
        // 새로운 기준은 기존 z의 부모로 설정하고
        void right_rotate(rbtree *t, node_t *z);
        // 라이트회전을 하면 부모로 설정된 z가
        // 이제 자식이 되고 원래 자식이 부모가 된다(z->parent)
        }
        z->parent->color = RBTREE_BLACK;
        z->parent->parent->color = RBTREE_RED;
        void left_rotate(rbtree *t, node_t *z);
      }
    }
  }
  t->root->color = RBTREE_BLACK;
  return t->root;
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  // TODO: implement insert
  node_t *z = (node_t *)calloc(1, sizeof(node_t));    // z는 새로운 노드
    z->key = key;
  node_t *y = t->nil;
  node_t *x = t->root;
  while (x != t->nil) {
    y = x;
    if (z->key < x->key)
      x = x->left;
    else
      x = x-> right;
  }
  z->parent = y;
  if (y == t->nil)
    t->root = z;
  else if (z->key < y->key)
    y->left = z; 
  else
    y->right = z;
  z->left = t->nil;
  z->right = t->nil;
  z->color = RBTREE_RED;
  return t->root;
  rbtree_insert_fixup(rbtree *t, node_t *z);
  }

node_t *rbtree_find(const rbtree *t, const key_t key) {
  // TODO: implement find
  node_t *n = t->root;
  while (n != t->nil || key != n->key) {
    if (key < n->key)
      n = n->left;
    else
      n = n->right;
  }
  if (n == t->nil)
    return NULL;
  else
    return n;
  // return t->root;
}

node_t *rbtree_min(const rbtree *t) {
  // TODO: implement find
  node_t *ptr = t->root;
  while (ptr != t->nil) {
    ptr = t->root->left;
  }
  return ptr;
}

node_t *rbtree_max(const rbtree *t) {
  // TODO: implement find
  node_t *ptr = t->root;
  while (ptr != t->nil) {
    ptr = t->root->right;
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
// *x 삭제 대상 포인터, x의 형제를 가리키는 포인터
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
  // TODO: implement erase
  // 여기서 p는 CLRS에서 나오는 z ; 삭제 대상
  node_t *y = p;
  color_t y_origin_color = y->color;
  node_t *x;
  if (p->left == t->nil) {
    x = p->right;
    rbtree_transplant(t, p, p->right);
  }
  else if (p->right == t->nil) {
    x = p->left;
    rbtree_transplant(t, p, p->left);
  } 
  else {
    // while: tree-minimum; p292 최소 키 찾아가기
    while (p->right->left != t->nil){
      p->right = p->right->left;
    }
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

  return 0;
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  // TODO: implement to_array
  return 0;
}
