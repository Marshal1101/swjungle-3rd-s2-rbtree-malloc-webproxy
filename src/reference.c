#include "rbtree.h"
#include <stdio.h>
#include <stdlib.h>


rbtree *new_rbtree(void) {
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  node_t *node_nil = (node_t *)calloc(1, sizeof(node_t));
  node_nil ->color = RBTREE_BLACK;
  node_nil ->left = NULL;
  node_nil ->right = NULL;
  p->root = node_nil;
  p->nil = node_nil;
  return p;
}

void del_node(rbtree *t, node_t *n){
  if(n != t->nil){
    del_node(t, n->left);
    del_node(t, n->right);
    free(n);
  }
}

void delete_rbtree(rbtree *t) {
  del_node(t, t->root);
  free(t->nil);
  free(t);
}
void left_rotate(rbtree *t, node_t *x){
  node_t *y = x->right;
  x->right = y->left;
  if(y->left != t->nil){
    y->left->parent = x;
  }
  y->parent = x->parent;
  if (x->parent == t->nil){
    t->root = y;
  }else if( x == x->parent->left){
    x->parent->left = y;
  }
  else {
    x->parent->right = y;
  }
  y->left = x;
  x->parent = y;
}
void right_rotate(rbtree *t, node_t *x){
  node_t *y = x->left;
  x->left = y->right;
  if(y->right != t->nil){
    y->right->parent = x;
  }
  y -> parent = x->parent;
  if(x->parent == t->nil){
    t->root = y;
  }else{
    if(x->parent->right == x){
      x->parent->right = y ;
    }else{
      x->parent->left = y;
    }
  }
  y->right = x;
  x->parent = y;
}

void rb_insert_fix(rbtree *t, node_t *n){
  node_t *u;
  // parent is black
  // 아래의 반복문을 거치지 않고 root를 검은색으로 변경 후 함수 종료
  while(n->parent->color == RBTREE_RED){
    // grandparent's left node is parent
    if (n->parent == n->parent->parent->left){
      u = n->parent->parent->right;
      // case 1: uncle is red
      if (u->color == RBTREE_RED){
        u->color = RBTREE_BLACK;
        n->parent->color = RBTREE_BLACK;
        n->parent->parent->color = RBTREE_RED;
        n = n->parent->parent;
      // case 2: uncle is black with triangle
      }else{ 
        if(n->parent->right == n){
          n = n->parent;
          left_rotate(t,n);
        }
        // case 3: uncle is black with line
        n->parent->parent->color = RBTREE_RED;
        n->parent->color = RBTREE_BLACK;
        right_rotate(t, n->parent->parent);

      }
    // grandparent's right node is parent
    }else{
      u = n->parent->parent->left;
      // case 1: uncle is red
      if (u->color == RBTREE_RED){
        u->color = RBTREE_BLACK;
        n->parent->color = RBTREE_BLACK;
        n->parent->parent->color = RBTREE_RED;
        n = n->parent->parent;
      // case 2: uncle is black with triangle
      }else{ 
        if(n->parent->left == n){
          n = n->parent;
          right_rotate(t, n);
        }// case 3: uncle is black with line
        n->parent->parent->color = RBTREE_RED;
        n->parent->color = RBTREE_BLACK;
        left_rotate(t, n->parent->parent);
      }
    }
  }
  t->root->color = RBTREE_BLACK;
}
node_t *rbtree_insert(rbtree *t, const key_t key) {
    node_t *new_node = (node_t*)calloc(1, sizeof(node_t));
    node_t *y = t->nil; // y
    node_t *x = t->root; // temp
    // 아래 반복문을 통해 p에는 새로 노드를 삽입할 곳의 부모노드가 들어오게 된다.
    while(x != t->nil){
        y = x;
        if(x->key > key){
            x = x->left;
        }else{
            x = x->right;
        }
    }
    new_node -> parent = y;
    if (y == t->nil){
      t->root = new_node;
    }else if(key < y->key){
      y->left = new_node;
    }else{
      y->right = new_node;
    }
    new_node->left = t->nil;
    new_node->right = t->nil;
    new_node->key = key;
    new_node->color = RBTREE_RED;
    rb_insert_fix(t, new_node);
  return new_node;
}

node_t *rbtree_find(const rbtree *t, const key_t key) {
  // TODO: implement find
  node_t *ptr;
  ptr = t->root;
  while(ptr != t->nil){
    if(ptr->key > key){
      ptr = ptr->left;
    }else if(ptr->key < key){
      ptr = ptr->right;
    }else{
      return ptr;
    }
  }
  return NULL;
}

node_t *rbtree_min(const rbtree *t) {
  // TODO: implement find
  node_t *ptr;
  ptr = t->root;
  if(ptr != t->nil){
    while(ptr ->left != t->nil){
      ptr = ptr->left;
    }
  }
  return ptr;
}
node_t *node_min(const rbtree *t, node_t *n) {
  // TODO: implement find
  node_t *ptr;
  ptr = n;
  while(ptr ->left != t->nil){
    ptr = ptr->left;
  }
  return ptr;
}


node_t *rbtree_max(const rbtree *t) {
  // TODO: implement find
  node_t *ptr;
  ptr = t->root;
  if(ptr != t->nil){
    while(ptr ->right != t->nil){
      ptr = ptr->right;
    }
  }
  return ptr;
}

void rb_transplant(rbtree *t, node_t *u, node_t *v){
  if(u->parent == t->nil){
    t->root = v;
  }else if(u == u->parent->left){
    u->parent->left = v;
  }else{
    u->parent->right = v;
  }
  v->parent = u->parent;
}

void rb_delete_fixup(rbtree *t, node_t *x){
  node_t *u;
  while(t->root != x && x->color == RBTREE_BLACK){
    if(x->parent->left == x){
      u = x->parent->right;
      // case1: uncle이 red
      if(u->color == RBTREE_RED){
        x->parent->color = RBTREE_RED;
        u->color = RBTREE_BLACK;
        left_rotate(t, x->parent);
        u = x->parent->right;
      }
      // case2: uncle이 black이고 두 자식이 모두 black
      if(u->left->color == RBTREE_BLACK && u->right->color == RBTREE_BLACK){
        u->color = RBTREE_RED;
        x = x->parent;
      }
      // case3: uncle이 black이고 왼쪽 자식은 red 오른쪽 자식은 black
      else{
        if(u->right->color == RBTREE_BLACK){
          u->color = RBTREE_RED;
          u->left->color = RBTREE_BLACK;
          right_rotate(t, u);
          u = x->parent->right;
        }
        // case4: uncle이 black이고 오른쪽 자식이 red
        u->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        u->right->color = RBTREE_BLACK;
        left_rotate(t, x->parent);
        x = t->root;
      }
    }else{
      u = x->parent->left;
      // case1: uncle이 red
      if(u->color == RBTREE_RED){
        x->parent->color = RBTREE_RED;
        u->color = RBTREE_BLACK;
        right_rotate(t, x->parent);
        u = x->parent->left;
      }
      // case2: uncle이 black이고 두 자식이 모두 black
      if(u->right->color == RBTREE_BLACK && u->left->color == RBTREE_BLACK){
        u->color = RBTREE_RED;
        x = x->parent;
      }
      // case3: uncle이 black이고 오른쪽 자식은 red 왼쪽 자식은 black
      else{ 
        if(u->left->color == RBTREE_BLACK){
          u->color = RBTREE_RED;
          u->right->color = RBTREE_BLACK;
          left_rotate(t, u);
          u = x->parent->left;
        }
        // case4: uncle이 black이고 왼쪽 자식이 red
        u->color = x->parent->color;
        x->parent->color = RBTREE_BLACK;
        u->left->color = RBTREE_BLACK;
        right_rotate(t, x->parent);
        x = t->root;
      }
    }
  }
  x->color = RBTREE_BLACK;
}
int rbtree_erase(rbtree *t, node_t *p) {
  // TODO: implement erase
  // 삭제 진행
  node_t *y = p;
  node_t *ptr; // x
  color_t y_color = p->color;
  if(p->left == t->nil){
    ptr = p->right;
    rb_transplant(t, p, p->right);
  }
  else if(p->right == t->nil){
    ptr = p->left;
    rb_transplant(t, p, p->left);
  }else{
    y = node_min(t, p->right);
    // printf("node_min->key: %d\n", y->key);
    y_color = y->color;
    ptr = y->right;
    if(y ->parent == p){
      ptr->parent = y;
    }else{
      rb_transplant(t, y, y->right);
      p->right->parent = y;
      y->right = p->right;
    }
    rb_transplant(t, p, y);
    y->left = p->left;
    p->left->parent = y;
    y->color = p->color;
  }
  if(y_color == RBTREE_BLACK){
    rb_delete_fixup(t, ptr);
  }
  free(p);
  return 0;
}
int node_to_array(const rbtree *t, const node_t *n, key_t *arr, int i){
  if(n->left != t->nil){
    i = node_to_array(t, n->left, arr, i);
  }
  arr[i] = n->key;
  i++;
  if(n->right != t->nil){
    i = node_to_array(t, n->right, arr, i);
  }
  return i;
}


int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  // TODO: implement to_array
  node_to_array(t, t->root, arr, 0);
  return 0;
}
