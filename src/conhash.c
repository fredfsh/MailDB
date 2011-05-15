
/* Copyright (C) 2010. sparkling.liang@hotmail.com. All rights reserved. */

#include "conhash.h"
#include "conhash_inter.h"

struct conhash_s* conhash_init(conhash_cb_hashfunc pfhash)
{
  /* alloc memory and set to zero */
  struct conhash_s *conhash =
      (struct conhash_s*) calloc(1, sizeof(struct conhash_s));

  do
  {
    if (!conhash) break;

    /* alloc memory for node list */
    conhash->nodes = (struct node_s *) calloc(1, sizeof(struct node_s));
    if (!conhash->nodes) break;

    /* setup callback functions */
    if(pfhash != NULL)
    {
        conhash->cb_hashfunc = pfhash;
    }
    else
    {
        conhash->cb_hashfunc = __conhash_hash_def;
    }
    util_rbtree_init(&conhash->vnode_tree);
    return conhash;

  }while(0);

  conhash_fini(conhash);
  return NULL;
}

void conhash_fini(struct conhash_s *conhash)
{
  if (conhash)
  {
    // Frees rb tree.
    while(!util_rbtree_isempty(&(conhash->vnode_tree)))
    {
        util_rbtree_node_t *rbnode = conhash->vnode_tree.root;
        util_rbtree_delete(&(conhash->vnode_tree), rbnode);
        __conhash_del_rbnode(rbnode);
    }
    // Frees node list.
    if (conhash->nodes)
    {
      struct node_s *p, *q;
      q = conhash->nodes;
      if (q)
      {
        p = q->next;
        while (p)
        {
          q->next = p->next;
          free(p);
          p = q->next;
        }
        free(q);
      }
    }
    free(conhash);
  }
}

// Transfers states of every idle node to "run".
// TODO: Recovery work such as loading dump file into memory.
void conhash_reset(struct conhash_s *conhash)
{
  struct node_s *p;

  if (!conhash) return;

  p = conhash->nodes->next;
  while (p)
  {
    p->state = NODE_STATE_RUN;
    p = p->next;
  }
}

// Adds a node.
// When a node with same @ip already exists in the ring, @returns 0
// immediately and @replica is ignored.
int conhash_add_node(struct conhash_s *conhash, const struct in_addr ip,
    const u_int replica)
{
  struct node_s *p;

  if (!conhash) return -1;

  // Checks node with same ip.
  p = conhash_get_node(conhash, ip);
  if (p) return 0;

  // Allocates memory for the added node.
  p = conhash->nodes;
  while (p->next) p = p->next;
  p->next = (struct node_s *) calloc(1, sizeof(struct node_s));
  if (!p->next) return -1;

  p = p->next;
  __ip_cpy(&p->ip, &ip);
  p->replicas = replica;
  p->state = NODE_STATE_IDLE;

  /* add replicas of server */
  __conhash_add_replicas(conhash, p);

  return 0;
}

// Removes a node whose ip = @ip.
int conhash_del_node(struct conhash_s *conhash, const struct in_addr ip)
{
  struct node_s *p, *q;

  if (!conhash) return -1;

  // Checks if the node is in the ring.
  p = conhash->nodes;
  q = p->next;
  while (q)
  {
    if (__ip_equals(&ip, &q->ip))
    {
      p->next = q->next;
      /* del replicas of server */
      __conhash_del_replicas(conhash, q);
      free(q);
      return 0;
    }
    p = q;
    q = p->next;
  }

  return 0;
}

// @returns node whose ip = @ip, or NULL when not found.
struct node_s * conhash_get_node(const struct conhash_s *conhash,
    const struct in_addr ip)
{
  struct node_s *p;

  p = conhash->nodes->next;
  while (p)
  {
    if (__ip_equals(&ip, &p->ip)) return p;
    p = p->next;
  }
  return NULL;
}

// Retrieves at most C servers responsible for the key and @returns their ips.
void conhash_lookup(struct conhash_s *conhash, const char *object, int *ipNum,
    struct in_addr *ips)
{
    int i;
    int num;
    long hash;
    const util_rbtree_node_t *rbnode, *startNode;
    struct node_s *node;
    struct in_addr ip;

    do
    {
        if((conhash==NULL) || (conhash->ivnodes==0) || (object==NULL)) break;
        /* calc hash value */
        hash = conhash->cb_hashfunc(object);
        //printf("[debug]conhash.c: hash = %lx\n", hash);  // debug
        
        rbnode = util_rbtree_geq_key(&conhash->vnode_tree, hash);
        if(rbnode == NULL) break;
        startNode = rbnode;
        if (((struct virtual_node_s *) rbnode->data)->node->state ==
            NODE_STATE_RUN) {
          num = 1;
          //printf("[debug]conhash.c: num = %d\n", num);  // debug
          __ip_cpy(&ips[0],
              &((struct virtual_node_s *) rbnode->data)->node->ip);
        } else {
          num = 0;
        }

        while (num < C)
        {
            rbnode = util_rbtree_gt_key(&conhash->vnode_tree, rbnode->key);
            if (rbnode == startNode) break;
            node = ((struct virtual_node_s *) rbnode->data)->node;
            if (node->state != NODE_STATE_RUN) continue;
            __ip_cpy(&ip, &node->ip);
            for (i = 0; i < num; ++i) {
              if (__ip_equals(&ips[i], &ip)) break;
            }
            if (i != num) continue;
            __ip_cpy(&ips[num++], &ip);
        }
        *ipNum = num;
        return;

    } while(0);

    *ipNum = 0;
}
