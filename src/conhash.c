
/* Copyright (C) 2010. sparkling.liang@hotmail.com. All rights reserved. */

#include "conhash.h"
#include "conhash_inter.h"

struct conhash_s* conhash_init(conhash_cb_hashfunc pfhash)
{
    /* alloc memory and set to zero */
    struct conhash_s *conhash =
        (struct conhash_s*) calloc(1, sizeof(struct conhash_s));
    if(conhash == NULL)
    {
        return NULL;
    }
    do
    {
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

    free(conhash);
    return NULL;
}

void conhash_fini(struct conhash_s *conhash)
{
    if(conhash != NULL)
    {
        /* free rb tree */
        while(!util_rbtree_isempty(&(conhash->vnode_tree)))
        {
            util_rbtree_node_t *rbnode = conhash->vnode_tree.root;
            util_rbtree_delete(&(conhash->vnode_tree), rbnode);
            __conhash_del_rbnode(rbnode);
        }
        free(conhash);
    }
}

void conhash_set_node(struct node_s *node, const struct in_addr ip,
    u_int replica)
{
    __ip_cpy(&node->ip, &ip);
    node->replicas = replica;
    node->flag = NODE_FLAG_INIT;
}

int conhash_add_node(struct conhash_s *conhash, struct node_s *node)
{
    if((conhash==NULL) || (node==NULL)) 
    {
        return -1;
    }
    /* check node fisrt */
    if(!(node->flag&NODE_FLAG_INIT) || (node->flag & NODE_FLAG_IN))
    {
        return -1;
    }
    node->flag |= NODE_FLAG_IN;
    /* add replicas of server */
    __conhash_add_replicas(conhash, node);
 
    return 0;
}

int conhash_del_node(struct conhash_s *conhash, struct node_s *node)
{
   if((conhash==NULL) || (node==NULL)) 
    {
        return -1;
    }
    /* check node first */
    if(!(node->flag&NODE_FLAG_INIT) || !(node->flag&NODE_FLAG_IN))
    {
        return -1;
    }
    node->flag &= (~NODE_FLAG_IN);
    /* add replicas of server */
    __conhash_del_replicas(conhash, node);

    return 0;
}

void conhash_lookup(struct conhash_s *conhash, const char *object, int *ipNum,
    struct in_addr *ips)
{
    int i;
    int num;
    long hash;
    const util_rbtree_node_t *rbnode, *startNode;
    struct in_addr ip;

    do
    {
        if((conhash==NULL) || (conhash->ivnodes==0) || (object==NULL)) break;
        /* calc hash value */
        hash = conhash->cb_hashfunc(object);
        
        rbnode = util_rbtree_geq_key(&conhash->vnode_tree, hash);
        if(rbnode == NULL) break;

        num = 1;
        __ip_cpy(&ips[0], &((struct virtual_node_s *) rbnode->data)->node->ip);
        startNode = rbnode;
        while (num < C)
        {
            rbnode = util_rbtree_gt_key(&conhash->vnode_tree, rbnode->key);
            if (rbnode == startNode) break;
            __ip_cpy(&ip, &((struct virtual_node_s *) rbnode->data)->node->ip);
            for (i = 0; i < num; ++i) {
              if (__ip_equals(&ips[i], &ip)) break;
            }
            if (i == num) __ip_cpy(&ips[num++], &ip);
        }
        *ipNum = num;
        return;

    } while(0);

    *ipNum = 0;
}
