/*
Implementation front-end for compiler design.

Following the Kaleidoscope project (LLVM)
 */

/*
example lenguage
# Compute the x'th fibonacci number.
def fib(x)
  if x < 3 then
    1
  else
    fib(x-1)+fib(x-2)

# This expression will compute the 40th number.
fib(40)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

/* Signals */
#define KS_OK 0;
#define KS_ERR 1;

/* Enum of data types */
enum Token {
  KSOBJ_TYPE_EOF /*Indicate that inputs is finished, so stop the parser*/,
  /*Commands*/
  KSOBJ_TYPE_DEF, /*Maps the function definition*/
  KSOBJ_TYPE_EXTERN , /* Declare an external function*/
  KSOBJ_TYPE_IDENTIFIER, /* Declare a functin or variable name*/
  KSOBJ_TYPE_NUMBER, /* Define double number*/
  KSOBJ_TYPE_OPERATOR, /* Define symbols */
  KSOBJ_TYPE_BUF 
};

typedef enum {
  AST_NUMBER,
  AST_VARIABLE,
  AST_BINARY,
  AST_CALL,
  AST_PROTOTYPE,
  AST_FUNCTION
}ASTNodeType;

/* ----------------------------------------------- DATA TYPE ------------------------------------------------- */
typedef struct ksobj{
  int refcount;
  int type;
  
  /*KS_OJB_TYPE*/
  union {
    double i;
    struct {
      char *ptr;
      size_t len;
    }str; /*For define simbols*/

    struct {
      struct ksobj **ptr;
      size_t len;
    }list;
  };
}ksobj;

typedef struct {
  /* beginning of token*/
  char *start;
  /* next token */ 
  char *p; 
}kstoknizer;

typedef struct ASTNode{
  int type;
  int refcount;
  union {
    double number;
    struct {
      char *s;
      size_t len;
    }str; /* Save string like: variable name */

    struct {
      char op;
      struct ASTNode *lsh;
      struct ASTNode *rsh;
    }binary;

  };
}ASTNode;

typedef struct ksctx{
  ksobj *stack;
  ASTNode *AST;
  int pos;
}ksctx;

/* ------------------------------------------------ FUNCTION PROTOTIPES -------------------------------------------- */
ksobj *listGetBy(ksobj *l, int pos);

/* ------------------------------------------------- WRAPPER FUNCTION ---------------------------------------------- */
void *safeRealloc(void *ptr, size_t size){

  void *newptr = realloc(ptr, size);
  if (newptr == NULL){
    fprintf(stderr, "Out of memory reallocation %zu bytes \n", size);
    exit(1);
  }
  return newptr;
}

#define safeMalloc(size) safeRealloc(NULL, size);
#define listCurr(list) listGetBy(list, -1);
/* --------------------------------------------- OBJECT CREATION FUINCTION ------------------------------------------ */
ksobj *createObject(int type){
  ksobj *o = safeMalloc(sizeof(*o));
  o->type = type;
  o->refcount = 1;
  return o;
}

ksobj *createNumberObj(double i){
  ksobj *o = createObject(KSOBJ_TYPE_NUMBER);
  o->i = i;
  return o;
}

/* Used to oprtator: '+', '*', '/', '-' and for KSOBJ_TYPE_EXTERN, KSOBJ_TYPE_OPERATOR */
ksobj *createSymbolObj(char *c, size_t len, int obj_type){
  ksobj *o = createObject(obj_type);
  o->str.ptr = safeMalloc(len+1);
  o->str.len = len;
  memcpy(o->str.ptr, c, len);
  o->str.ptr[len] = 0;
  return o;
}

ksobj *createListObj(void){
  ksobj *o = createObject(KSOBJ_TYPE_BUF);
  o->list.ptr = NULL;
  o->list.len = 0;
  return o;
}

void retain(ksobj *o){
  o->refcount++;
}

void release(ksobj *o){
  assert(o->refcount > 0);
  o->refcount--;
  if(o->refcount == 0) free(o);
}

void printObj(ksobj *o){
  switch(o->type){
   
  case KSOBJ_TYPE_NUMBER:
    printf("%f", o->i);
    break;
  case KSOBJ_TYPE_IDENTIFIER:
  case KSOBJ_TYPE_EXTERN:
  case KSOBJ_TYPE_DEF:
  case KSOBJ_TYPE_OPERATOR:
    printf("%s", o->str.ptr);
    break;
  case KSOBJ_TYPE_BUF:
    printf("[");
    for(size_t j = 0; j<o->list.len; j++){
      ksobj *ele = o->list.ptr[j];
      printObj(ele);
    }
    printf("]\n");
    break;
  default:
    printf("?");
    break;
  }
}

void printAST(ASTNode *abs){
  switch (abs->type){

  case AST_NUMBER:
    printf("%f ", abs->number);
    break;
  case AST_VARIABLE:
    printf("%s ", abs->str.s);
    break;
  case AST_BINARY:{
    ASTNode *temp = abs;
    printf("(");
    printAST(temp->binary.lsh);
    printf(" %c ", abs->binary.op);
    printAST(temp->binary.rsh);
    printf(")");
    
    break;
    printf("\n");
  }

  default:
    printf("?\n");
    break;
  }
}

/* Add a new element at the end of the List 'list'.
 * We are allocationg the size of the point object {just one} times the number of new 'tfobj' elementes to add */
int compareStringObj(ksobj *a, ksobj *b){
  size_t minlen = a->str.len < b->str.len ? a->str.len : b->str.len;
  int cmp = memcmp(a->str.ptr, b->str.ptr, minlen); /* Zero if two strings are equal*/

  if (cmp == 0){
    if (a->str.len == b->str.len) return 0;
    else if (a->str.len > b->str.len) return 1;
    else return -1;
  }else{
    /* Case 'a' minor of 'b'*/
    if (cmp < 0) return -1;
    else return -1;
  }
}

/* -------------------------------------------- AST OBJECT FUNCTIONS --------------------------------------------------*/
ASTNode *createNode(int type){
  ASTNode *o = safeMalloc(sizeof(*o));
  o->type = type;
  o->refcount++;
  return o; 
}

ASTNode *createNumberNode(double val){
  ASTNode *o = createNode(AST_NUMBER);
  o->number = val;
  return o;
}

ASTNode *createBynaryNode(char op, struct ASTNode *left, struct ASTNode *right){
  ASTNode *o = createNode(AST_BINARY);
  o->binary.op = op;
  o->binary.lsh = left;
  o->binary.rsh = right;
  return o;
}

/* For variable and function name */
ASTNode *createIdentifierNode(char *s, size_t len){
  ASTNode *o = createNode(AST_VARIABLE);
  o->str.s = safeMalloc(len + 1);
  o->str.len = len;
  memcpy(o->str.s, s, len);
  o->str.s[len] = 0;
  return o;
}


/* -------------------------------------------- LIST OBJECT FUNCTIONS -------------------------------------------------*/
void listPush(ksobj *l, ksobj *o){
  l->list.ptr = safeRealloc(l->list.ptr, sizeof(ksobj *) * (l->list.len+1));
  l->list.ptr[l->list.len] = o;
  l->list.len++; 
}

ksobj *listPop(ksobj *l){
  if (l->list.len > 0)
   return l->list.ptr[l->list.len --];

  else {
    printf("Empty list!\n");
    return NULL;
  }
}

ksobj *listGetBy(ksobj *l, int pos){
  if (!l || l->type != KSOBJ_TYPE_BUF)
    return NULL;

  if (l->list.len == 0)
    return NULL;
 
  if (pos == -1 ){
    /*get the current position*/
    return l->list.ptr[l->list.len - 1];
  } else if (pos < 0 || pos >= l->list.len){
    return NULL;
  }
  
    return l->list.ptr[pos];
  
}

int getMatchPrecedance(char op){
  switch (op){
    case('+'): return 10;
    case('-'): return 10;
    case('*'): return 20;
    case('/'): return 20;
    default: return -1;
  }
}

/* ------------------------------------------------- PARSER FUNCTIONS -------------------------------------------------- */
ASTNode *parseNumber(ksctx *ctx){
  ksobj *tok = listGetBy(ctx->stack, ctx->pos++); /* Consume the current result*/
  return createNumberNode(tok->i);
}

ASTNode *parsePrimary(ksctx *ctx){
  ksobj *tok = listGetBy(ctx->stack, ctx->pos); /* At the beginning get back the '0' position*/

  if (!tok){
    fprintf(stderr, "Unexpected end of input\n");
    return NULL;
  }
  
  if (tok->type == KSOBJ_TYPE_NUMBER)
    return parseNumber(ctx);

  /* Variable case */

  if (tok->type ==  KSOBJ_TYPE_IDENTIFIER){
    ctx->pos++;
    return createIdentifierNode(tok->str.ptr, tok->str.len);
  }
  
  fprintf(stderr, "Errror: parse primary! \n");
  return NULL;
}

ASTNode *parseBinary(ASTNode *lsh, int exprPrec, ksctx *ctx){
  while(1){
    ksobj *tok = listGetBy(ctx->stack, ctx->pos);

    if (!tok || tok->type != KSOBJ_TYPE_OPERATOR )
      return lsh;

    char op = tok->str.ptr[0];
    int tok_prec = getMatchPrecedance(op);
					   
    if (tok_prec < exprPrec)
      return lsh;

    ctx->pos++; /* Consume operator */

    ASTNode *rsh = parsePrimary(ctx);

    ksobj *next_tok = listGetBy(ctx->stack, ctx->pos);
    if (next_tok && next_tok -> type == KSOBJ_TYPE_OPERATOR){
      int next_prec = getMatchPrecedance(next_tok->str.ptr[0]);
      if ( tok_prec < next_prec)
	rsh = parseBinary(rsh, tok_prec+1, ctx);
    }
    
    lsh = createBynaryNode(op, lsh, rsh);
  }
}

ASTNode *parseExpression(ksctx *ctx){
  ASTNode *lsh = parsePrimary(ctx);
  return  parseBinary(lsh, 0, ctx);
}

/* check if the current pointed value is a space or not*/
void parseSpaces(kstoknizer *par){
  while(isspace(par->p[0])) par->p++; 
}

/* Check is a character is on the given string*/
int isPresentOperator(char c){
  char operators[] = "+-*/%";
  return strchr(operators, c) != NULL;
}

ksobj *parseSymbol(kstoknizer *tok){
  char *start = tok->p;
  while(tok->p[0] && isPresentOperator(tok->p[0])) tok->p++;
  int len = tok->p - start;
  return createSymbolObj(start, len, KSOBJ_TYPE_OPERATOR);
}

int isIdentifierChar(char c){
  return isalpha(c);
}

void parseComments(kstoknizer *tok){
  while(tok->p[0] && tok->p[0] != '\n' && tok->p[0] != '\r'){
    tok->p++;
  }
}

ksobj *parseIdentifier(kstoknizer *tok){
  char *start = tok->p;

  while(tok->p[0] && isIdentifierChar(tok->p[0])){
    tok->p++;
  }
  size_t len = tok->p - start;

  /* Check functions */
  if (len == 3 && (strncmp(start, "def", len)) == 0)
    return createSymbolObj(start, len,  KSOBJ_TYPE_OPERATOR);
  if (len == 6 && (strncmp(start, "extern", len)) == 0)
    return createSymbolObj(start, len, KSOBJ_TYPE_EXTERN);

  /* Otherwise is a normal identifier, like varniable*/
  return createSymbolObj(start, len, KSOBJ_TYPE_IDENTIFIER);
}

/* ------------------------------------------------ SCANNER TOKENIZER -------------------------------------------------- */
ksobj *compile(char *prgtext){
  kstoknizer tokenizer;
  tokenizer.start = prgtext;
  tokenizer.p = prgtext;

  ksobj *tokenized = createListObj();
  while(tokenizer.p){
    ksobj *o;
    char *token_start = tokenizer.p;

    parseSpaces(&tokenizer);
    if (tokenizer.p[0] == 0) break; /* end of file*/
    /* Check double number*/
    if (isdigit(tokenizer.p[0]) || (tokenizer.p[0] == '.' && isdigit(tokenizer.p[1]))){
      char *endptr;
      double val = strtod(tokenizer.p, &endptr);
      o = createNumberObj(val);
      tokenizer.p = endptr;
    }else if (isIdentifierChar(tokenizer.p[0])){
      o = parseIdentifier(&tokenizer);
      
    }else if ((isPresentOperator(tokenizer.p[0]))){
      o = parseSymbol(&tokenizer);
    }else if (strchr("#", tokenizer.p[0])) {
      parseComments(&tokenizer);
      continue;
    }else{
      o = NULL;
    }
    /* Check if the current token produce an error, in this case we have to free all object untile Now*/
    if (o == NULL){
      release(o);
      printf("Syntax Error near to %32s ....\n", token_start);
      return NULL;
    }else{
      /* Add the token to the list*/
      listPush(tokenized, o);
    }
  }

  return tokenized;

}

/* -------------------------------------------------- EXECUTION AND CONTEXT ---------------------------------------- */
ksctx* createContext(void){
  ksctx *ctx = safeMalloc(sizeof(*ctx));
  ctx->stack = createListObj();
  ctx->AST = NULL;
  ctx->pos = 0;

  return ctx;
}

void exec(ksctx *ctx, ksobj *prg){
  assert(prg->type == KSOBJ_TYPE_BUF);
  for (size_t j = 0; j < prg->list.len; j++){
    ksobj *word = prg->list.ptr[j];
    switch(word->type){

      default:
       listPush(ctx->stack, word);
       retain(word);
       break;
     }
   
  }
  ctx->AST =  parseExpression(ctx);
  
}

/* -------------------------------------------------------- MAIN ----------------------------------------------------- */
int main(int argc, char **argv){
  if (argc != 2){
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    return KS_ERR;
  }
  FILE *fd = fopen(argv[1], "r");
  if (fd == NULL){
    fprintf(stderr, "ERROR: Kaledoscope impossible to open file\n");
    return KS_ERR;
  }
  /* Move the indicator from the beginnig to the end and vice versa.
    Find the dimension of the file.
  */
  fseek(fd, 0, SEEK_END);
  long file_size = ftell(fd);
  fseek(fd, 0, SEEK_SET);
  char *code_text = safeMalloc(file_size+1);

  /* Fondamental for passing a data from rigid memory to physical one */
  fread(code_text, file_size, 1, fd );
  code_text[file_size] = 0;
  fclose(fd);

  printf("program content:  %s \n", code_text);
  
  ksobj *prg = compile(code_text);
  
  if (prg == NULL){
    fprintf(stderr, "ERROR: compile typer\n");
    free(code_text);
    return KS_ERR;
  }

  ksctx *ctx = createContext();
  exec(ctx, prg);

  printObj(ctx->stack);

  printAST(ctx->AST);
  
  return 0;

}
