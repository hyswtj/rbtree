/***************************************************************
  Copyright (c) 2019 ShenZhen Panath Technology, Inc.

  The right to copy, distribute, modify or otherwise make use
  of this software may be licensed only pursuant to the terms
  of an applicable ShenZhen Panath license agreement.
 ***************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include "rbtree.h"

#define CHECK_INSERT 1    // "插入"动作的检测开关(0，关闭；1，打开)
#define CHECK_DELETE 1    // "删除"动作的检测开关(0，关闭；1，打开)

typedef int KEY;

struct test_rb_node {
    struct rb_node rb_node;
    KEY key;
};

/* test mode, 1 for function test, 2 for performance test */
static int test_mode = 0;

/* node number, default 3 */
static int nodes_num = 3;

/* perf loop counts, default 1, valid for performance test */
static int perf_loops = 1;

static char *progname;

/* test keys */
static KEY *test_keys;

/* test red black tree */
static struct rb_root test_rb_tree = RB_ROOT;

/* test red black node data */
static struct test_rb_node *rb_node_data;

static inline unsigned long _rdtsc(void)
{
	union {
		unsigned long tsc_64;
		struct {
			unsigned int lo_32;
			unsigned int hi_32;
		};
	} tsc;

	asm volatile("rdtsc" :
		     "=a" (tsc.lo_32),
		     "=d" (tsc.hi_32));
	return tsc.tsc_64;
}

int rb_compare(struct rb_node *node, void *key)
{
    struct test_rb_node *test_node = (struct test_rb_node *)node;
    KEY *key_val = (KEY *)key;
    KEY node_val = test_node->key;

    if (*key_val < node_val) {
        return -1;
    }
    else if (*key_val > node_val) {
        return 1;
    }
    else {
        return 0;
    }
}

int test_rb_insert(struct rb_root *root, int index)
{
    struct test_rb_node *test_node = &rb_node_data[index];
    KEY key = test_node->key;

    return rb_insert(root, &test_node->rb_node, &key, rb_compare);
}

void test_rb_delete(struct rb_root *root, int index)
{
    rb_delete(root, &test_keys[index], rb_compare);
}

/*
 * 打印"红黑树"
 */
static void print_rbtree(struct rb_node *tree, KEY key, int direction)
{
    if(tree != NULL)
    {
        if(direction==0)    // tree是根节点
            printf("%2d(B) is root\n", key);
        else                // tree是分支节点
        {
            struct rb_node *parent = rb_parent(tree);
            printf("%2d(%s) is %2d's %6s child\n",
                key, rb_is_black(tree)?"B":"R",
                rb_entry(parent, struct test_rb_node, rb_node)->key,
                direction==1?"right" : "left");
        }

        if (tree->rb_left) {
            print_rbtree(tree->rb_left, rb_entry(tree->rb_left,
                struct test_rb_node, rb_node)->key, -1);
        }
        if (tree->rb_right) {
            print_rbtree(tree->rb_right,rb_entry(tree->rb_right,
                 struct test_rb_node, rb_node)->key,  1);
        }
    }
}

void test_rb_print(struct rb_root *root)
{
    if (root!=NULL && root->rb_node!=NULL) {
        print_rbtree(root->rb_node,
            rb_entry(root->rb_node, struct test_rb_node, rb_node)->key,  0);
    }
}

static void usage()
{
    printf(("\n  Usage:"
       " %s will do compare test of avl and red black tree with same data."
       "\n  %s <test mode> <nodes number> [perf loops]\n\n"
       "  options:\n"
       "  test mode    : 1 for function test, 2 for perf test.\n"
       "  nodes number : test nodes number, at least 3\n"
       "  perf loops   : perf loops, default is 1\n\n"
       ), progname, progname);
    exit(0);
}

static void parse_params(int argc, char *argv[])
{
    int tmp;

    /* test mode */
    tmp = atoi(argv[1]);
    if ((tmp <= 0) || (tmp > 2)) {
        usage();
    }
    test_mode = tmp;

    /* nodes number */
    tmp = atoi(argv[2]);
    if (tmp < 3) {
        usage();
    }
    nodes_num = tmp;

    /* perf loops */
    if (argc == 4) {
        if (test_mode == 2) {
            tmp = atoi(argv[3]);
            if (tmp < 3) {
                usage();
            }
            perf_loops = tmp;
        }
        else {
            usage();
        }
    }

    if (argc > 4) {
        usage();
    }

    printf("-----------------------------------------------------------\n");
    printf("                  Red Black Tree test demo                 \n");
    printf("      test mode    :     %s test\n",
        ((1 == test_mode) ? "function" : "performance"));
    printf("      nodes number :     %d \n", nodes_num);
    printf("      perf loops   :     %d \n", perf_loops);
    printf("-----------------------------------------------------------\n");
}

void test_data_build(int show)
{
    int i;
    int print_num = (nodes_num > 50) ? (50) : (nodes_num);

    test_keys = (KEY *)malloc(nodes_num * sizeof(KEY));
    if (NULL == test_keys) {
        printf("Build test keys failed, no enough memory, nodes_num is %d\n",
            nodes_num);
        exit(1);
    }

    for(i = 0; i < nodes_num; ++i) {
        test_keys[i] = rand()%100000;
    }

    rb_node_data = (struct test_rb_node *)
        malloc(sizeof(struct test_rb_node) * nodes_num);
    if (rb_node_data == NULL) {
        printf("Build test rbtree failed, no enough memory, nodes_num is %d\n",
            nodes_num);
        exit(1);
    }

    for(i = 0; i < nodes_num; ++i) {
        rb_node_data[i].key = test_keys[i];
    }

    if (!show) {
        return;
    }
    printf("-----------------------------------------------------------\n");
    printf("Test keys(nodes_num:%d, print_num:%d):\n", nodes_num, print_num);
    for(i = 0; i < print_num; ++i) {
        printf("%u ", test_keys[i]);
        if((i & 0x7) == 0x7) {
            printf("\n");
        }
    }
    printf("\n-----------------------------------------------------------\n");
}

void test_data_free()
{
    free(test_keys);
    free(rb_node_data);
    test_rb_tree = RB_ROOT;
}

void func_test()
{
    int i = 0;

    printf("--------------------Red Black Tree-------------------------\n");
    for (i = 0; i < nodes_num; ++i)
    {
        test_rb_insert(&test_rb_tree, i);
#if CHECK_INSERT
        printf("== Add node: %d\n", test_keys[i]);
        printf("== Tree detail: \n");
        test_rb_print(&test_rb_tree);
        printf("\n");
#endif

    }

#if CHECK_DELETE
    for (i = 0; i < nodes_num; ++i) {
        test_rb_delete(&test_rb_tree, i);

        printf("== Del node: %d\n", test_keys[i]);
        printf("== Tree detail: \n");
        test_rb_print(&test_rb_tree);
        printf("\n");
    }
#endif
}

void perf_test()
{
    int i = 0;
    int j = 0;
    unsigned long cost1;
    unsigned long cost2;
    unsigned long time1;
    unsigned long time2;

    printf("-----------------------Perf test---------------------------\n");

	for (i = 0; i < perf_loops; i++) {
        test_data_build(0);
        time1 = _rdtsc();
		for (j = 0; j < nodes_num; j++) {
			test_rb_insert(&test_rb_tree, j);
		}
        time2 = _rdtsc();
        cost1 = time2 - time1;

        time1 = _rdtsc();
		for (j = 0; j < nodes_num; j++) {
			test_rb_delete(&test_rb_tree, j);
		}
        time2 = _rdtsc();
        cost2 = time2 - time1;
        printf("[ rb]i:%d, insert cost:%lu, delete cost:%lu.\n",
            i, cost1, cost2);

        test_data_free();
        printf("-----------------------------------------------------------\n");
	}
}

int main(int argc, char *argv[])
{
    if (!(progname = strrchr(argv[0], '/'))) {
        progname = argv[0];
    }
    else {
        progname++;
    }

    if (argc < 3) {
        usage();
    }

    parse_params(argc, argv);
    srand( (unsigned)time( NULL ) );

    if (test_mode == 1) {
        test_data_build(1);
        func_test();
        test_data_free();
    }
    else {
        perf_test();
    }
    return 0;
}
