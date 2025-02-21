/***************************************************************************************
* Copyright (c) 2014-2022 Zihao Yu, Nanjing University
*
* NEMU is licensed under Mulan PSL v2.
* You can use this software according to the terms and conditions of the Mulan PSL v2.
* You may obtain a copy of Mulan PSL v2 at:
*          http://license.coscl.org.cn/MulanPSL2
*
* THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
* EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
* MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
*
* See the Mulan PSL v2 for more details.
***************************************************************************************/

#include <isa.h>

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <regex.h>
#include <string.h>
#include <common.h>

/*The type of token*/
enum {
  TK_NOTYPE = 256, TK_EQ,
  TK_NUM,
  /* TODO: Add more token types */

};

/*Define the type and relevant rule of every regular experession*/
static struct rule {
  const char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},    // spaces
  {"\\+", '+'},         // plus
  {"-", '-'},           // minus
  {"\\*", '*'},         // multiply
  {"/", '/'},           // divide
  {"==", TK_EQ},        // equal
  {"\\(", '('},         // left parenthesis
  {"\\)", ')'},         // right parenthesis
  {"[0-9]+", TK_NUM},   // number
};

#define NR_REGEX ARRLEN(rules)

static regex_t re[NR_REGEX] = {};

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    /* Transfer the regular experession of string format into binary format*/
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

#define TOKEN_LEN 32
typedef struct token {
  int type;
  char str[TOKEN_LEN];
} Token;

#define NR_TOKEN 32

/*Store the parsed tokens*/
static Token tokens[NR_TOKEN] __attribute__((used)) = {};
static int nr_token __attribute__((used))  = 0;

enum expr_t {BAD_EXPR, GOOD_EXPR}; 

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);

        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */
        Assert(nr_token <= NR_TOKEN, "Token array has been full!");
        switch (rules[i].token_type) {          
          case TK_NUM:
            Assert(substr_len <= TOKEN_LEN, "The length of token of TK_NUM is too long.");
            tokens[nr_token].type = rules[i].token_type;
            memcpy(tokens[nr_token].str, substr_start, substr_len);
            nr_token++;
            break;

          case TK_NOTYPE:
            break;
          
          default: 
            tokens[nr_token].type = rules[i].token_type;
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  return true;
}

/*Determine whether the expression is surrounded by a pair of matching parentheses, 
  and also check whether the left and right parentheses of the expression match.*/
bool check_parenthesis(int p, int q){
  assert(p<q);
  if (tokens[p].type != '(' || tokens[q].type != ')')
  {
    return false;
  }
  int cnt = 0;
  for(int i = p; i <= q; i++){
    if (tokens[i].type == '(')
      cnt++;
    else if (tokens[i].type == ')')
      cnt--; 

    if(cnt == 0 && i != q)
      return false;
  }

  assert(cnt == 0);
  return true;
}

/*Find the primary operator in the expression*/
int find_op(int p, int q){
  assert(p<q);
  int parenthrsis_cnt = 0;
  int op = -1;
  for (int i = p; i <= q; i++)
  {
    if (tokens[i].type == '(')
    {
      parenthrsis_cnt++;
    } else if (tokens[i].type == ')')
    {
      parenthrsis_cnt--;
    } else
    {
      if (parenthrsis_cnt == 0){
        /*Ensure the op is not inside a pair of parenthesis*/
        if (tokens[i].type == '+' || tokens[i].type == '-')
        {
          op = i;
        } else if(tokens[i].type == '*' || tokens[i].type == '/')
        {
          if(op == -1 || (tokens[op].type != '+' && tokens[op].type != '-'))
          {
            /*Ensure the primary operator's priority is lowest*/
            op = i;
          }
        }
      }
    }
  }
  return op;
}

enum expr_t eval(int p, int q, int* result){
  assert(p <= q);
  if (p == q)
  {
    assert(tokens[p].type == TK_NUM);
    *result = atoi(tokens[p].str);
    return GOOD_EXPR;
  } else if (check_parenthesis(p, q) == true)
  {
    printf("Delete one pair of parenthesis\n");
    return eval(p+1, q-1, result);
  } else{
    int op_position = find_op(p, q);
    printf("p=%d, q=%d, op_position=%d\n", p, q, op_position);
    assert(p <= op_position && op_position <= q);

    int left_result, right_result;
    enum expr_t left_ret = eval(p, op_position - 1, &left_result);
    enum expr_t right_ret = eval(op_position + 1, q, &right_result);

    assert(left_ret == GOOD_EXPR && right_ret == GOOD_EXPR);

    switch (tokens[op_position].type)
    {
    case '+':
      /* code */
      *result = left_result + right_result;
      break;
    case '-':
      *result = left_result - right_result;
      break;
    case '*':
      *result = left_result * right_result;
      break;
    case '/':
      assert(right_result != 0);
      *result = left_result / right_result;
      break;
    default:
      assert(0);
      break;
    }
  }

  return GOOD_EXPR;
  
}

word_t expr(char *e, bool *success) {
  printf("The expression is %s\n", e);
  if (!make_token(e)) {
    *success = false;
    return 0;
  }
  int result;
  enum expr_t ret = eval(0, nr_token - 1, &result);
  assert(ret == GOOD_EXPR);
  *success = true;
  printf("The result of the expression is %d\n", result);

  return 0;
}
