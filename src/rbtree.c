#include "rbtree.h"

#include <stdlib.h>

rbtree *new_rbtree(void) {
  rbtree *p = (rbtree *)calloc(1, sizeof(rbtree));
  // TODO: initialize struct if needed
  p->root->color = RBTREE_BLACK;
  p->nil->color = RBTREE_BLACK;
  return p;
}

void delete_rbtree(rbtree *t) {
  // TODO: reclaim the tree nodes's memory
  free(t);
}

node_t *rbtree_insert(rbtree *t, const key_t key) {
  // TODO: implement insert
  node_t *z = (node_t *)calloc(1, sizeof(node_t));      // z는 새로운 노드
    z->key = key;
  node_t *y = t->nil;
  node_t *x = t->root;
  while (x != t->nil) {
    y = x;
    if (z->key < x->key) {
      x = x->left;
    }
    else {
      x = x-> right;
    }
  }
  if (y == t->nil) {
    t->root = z;
  }
  else if (z->key < y->key) {
    y->left = z; 
  }
  else {
    y->right = z;
  }
  z->left = t->nil;
  z->right = t->nil;
  z->color = RBTREE_RED;
  return t->root;
  node_t *rbtree_insert_fixup(rbtree *t, node_t z);
}

node_t *rbtree_insert_fixup(rbtree *t, node_t *new_node) {
  while (new_node->parent->color = )
}


node_t *rbtree_find(const rbtree *t, const key_t key) {
  // TODO: implement find
  return t->root;
}

node_t *rbtree_min(const rbtree *t) {
  // TODO: implement find
  return t->root;
}

node_t *rbtree_max(const rbtree *t) {
  // TODO: implement find
  return t->root;
}

int rbtree_erase(rbtree *t, node_t *p) {
  // TODO: implement erase
  return 0;
}

int rbtree_to_array(const rbtree *t, key_t *arr, const size_t n) {
  // TODO: implement to_array
  return 0;
}
