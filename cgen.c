/****************************************************/
/* Lab de Compiladores - Prof. Galvão               */
/* File: cgen.c                                     */
/* The code generator implementation                */
/* for the C- compiler                              */
/* Adapted from:                                    */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "code.h"
#include "cgen.h"
#include "parse.h"
#include "analyze.h"
#include "assembly.h"

/* tmpOffset is the memory offset for temps
   It is decremented each time a temp is
   stored, and incremeted when loaded again
*/
static int tmpOffset = 0;

/* prototype for internal recursive code generator */
static void cGen (TreeNode * tree);


QuadList head = NULL;

int location = 0;
int mainLocation;

int nlabel = 0;
int ntemp = 0;
int nparams = -1;

Address aux;
Address var;
Address offset;
Address empty;

/* verificar esse vetor */
const char * OpKindNames[] =  { "add", "sub", "mult", "div", "lt", "lequal", "gt", "grequal", "and", "or", "assign", "alloc", "immed", "load", "store",
                              "vec", "goto", "iff", "ret", "fun", "end", "param", "call", "arg", "lab", "hlt" };

void quad_insert (OpKind op, Address addr1, Address addr2, Address addr3) {
  //printf("entrou aqui \n");
  Quad quad;
  quad.op = op;
  quad.addr1 = addr1;
  quad.addr2 = addr2;
  quad.addr3 = addr3;
  QuadList new = (QuadList) malloc(sizeof(struct QuadListRec));
  new->location = location;
  new->quad = quad;
  new->next = NULL;
  if (head == NULL) {
    head = new;
  }
  else {
    QuadList q = head;
    while (q->next != NULL) q = q->next;
    q->next = new;
  }
  location ++;
}

int quad_update(int loc, Address addr1, Address addr2, Address addr3) {
  QuadList q = head;
  while (q != NULL) {
    if (q->location == loc) break;
    q = q->next;
  }
  if (q == NULL) 
    return 0;
  else {
    q->quad.addr1 = addr1;
    q->quad.addr2 = addr2;
    q->quad.addr3 = addr3;
    return 1;
  }
}

char * newLabel() {
  //char label[nlabel_size + 2];
  char * label = (char *) malloc((nlabel_size + 3) * sizeof(char));
  sprintf(label, "L%d", nlabel);
  nlabel++;
  return label;
}

char * newTemp() {
  //char temp[ntemp_size + 2];
  char * temp = (char *) malloc((ntemp_size + 3) * sizeof(char));
  sprintf(temp, "$t%d", ntemp);
  ntemp = (ntemp + 1) % nregtemp;
  return temp;
}

Address addr_createEmpty() {
  Address addr;
  addr.kind = Empty;
  addr.contents.var.name = NULL;
 // addr.contents.var.scope = NULL;
  return addr;
}

Address addr_createIntConst(int val) {
  Address addr;
  addr.kind = IntConst;
  addr.contents.val = val;
  return addr;
}

Address addr_createString(char * name) {
  Address addr;
  addr.kind = String;
  addr.contents.var.name = (char *) malloc(strlen(name) * sizeof(char));
  strcpy(addr.contents.var.name, name);
  return addr;
}

/* Procedure genStmt generates code at a statement node */
static void genStmt( TreeNode * tree)
{ TreeNode * p1, * p2, * p3;
  Address addr1, addr2, addr3;
  Address aux1, aux2;
  int loc1, loc2, loc3;
  char * label;
  char * temp;
  //printf("%d \n",tree->kind.stmt);
  switch (tree->kind.stmt) {
    
    case IfK:
      if (TraceCode) emitComment("-> if");
      p1 = tree->child[0] ;
      p2 = tree->child[1] ;
      p3 = tree->child[2] ;
      // condicao if
      cGen(p1);
      addr1 = aux;
      // se condição falsa?
      loc1 = location;
      quad_insert(opIFF, addr1, empty, empty);
      // if
      cGen(p2);
      // vai para o final?
      loc2 = location;
      quad_insert(opGOTO, empty, empty, empty);
      // fim do if
      label = newLabel();
      quad_insert(opLAB, addr_createString(label/*, tree->scope*/), empty, empty);
      // se condição falsa vem pra cá
      quad_update(loc1, addr1, addr_createString(label/*, tree->scope*/), empty);
      // else
      cGen(p3);
      if (p3 != NULL) {
        // vai para o final?
        loc3 = location;
        quad_insert(opGOTO, empty, empty, empty);
      }
      label = newLabel();
      // final
      quad_insert(opLAB, addr_createString(label/*, tree->scope*/), empty, empty);
      quad_update(loc2, addr_createString(label/*, tree->scope*/), empty, empty);
      if (p3 != NULL)
        quad_update(loc3, addr_createString(label/*, tree->scope*/), empty, empty);
      if (TraceCode)  emitComment("<- if");
      break;

    case WhileK:
      if (TraceCode) emitComment("-> while");
      p1 = tree->child[0] ;
      p2 = tree->child[1] ;
      // inicio do while
      label = newLabel();
      quad_insert(opLAB, addr_createString(label/*, tree->scope*/), empty, empty);
      // condicao while
      cGen(p1);
      addr1 = aux;
      // se condição falsa?
      loc1 = location;
      quad_insert(opIFF, addr1, empty, empty);
      // while
      cGen(p2);
      loc3 = location;
      quad_insert(opGOTO, addr_createString(label/*, tree->scope*/), empty, empty);
      // final
      label = newLabel();
      quad_insert(opLAB, addr_createString(label/*, tree->scope*/), empty, empty);
      // se condição falsa vem pra cá
      quad_update(loc1, addr1, addr_createString(label/*, tree->scope*/), empty);
      if (TraceCode)  emitComment("<- while");
      break;

    case AssignK:
      if (TraceCode) emitComment("-> assign");
      p1 = tree->child[0];
      p2 = tree->child[1];
      // var
      cGen(p1);
      addr1 = aux;
      aux1 = var;
      aux2 = offset;
      // exp
      cGen(p2);
      addr2 = aux;
      quad_insert(opASSIGN, addr1, addr2, empty);
      quad_insert(opSTORE, aux1, aux2, addr1);
      if (TraceCode)  emitComment("<- assign");
      break;

    case ReturnK:
      if (TraceCode) emitComment("-> return");
      p1 = tree->child[0];
      cGen(p1);
      // se tem retorno
      if (p1 != NULL)
        addr1 = aux;
      //se não tem retorno
      else
        addr1 = empty;
      quad_insert(opRET, addr1, empty, empty);
      if (TraceCode) emitComment("<- return");
      break;

    default:
      break;
  }
} /* genStmt */

/* Procedure genExp generates code at an expression node */
static void genExp( TreeNode * tree)
{ TreeNode * p1, * p2, * p3;
  Address addr1, addr2, addr3;
  int loc1, loc2, loc3;
  char * label;
  char * temp;
  char * s = "";
  //printf("entrou no genExp \n");
 // printf("%d \n",tree->kind.exp);
  switch (tree->kind.exp) {
    case ConstK:
     // printf("ConstK \n");
      if (TraceCode) emitComment("-> Const");
      addr1 = addr_createIntConst(tree->attr.val);
      temp = newTemp();
      aux = addr_createString(temp);
      quad_insert(opIMMED, aux, addr1, empty);
      if (TraceCode)  emitComment("<- Const");
      break;
    
    case IdK:
      //printf("IdK \n");
      if (TraceCode) emitComment("-> Id");
      //temp = newTemp();
      aux = addr_createString(tree->attr.name/*, tree->scope*/);
     // printf("tree size: %d \n", tree->size);
      p1 = tree->child[0];
      if (p1 != NULL) {
//      printf("entrou no vec \n");
        temp = newTemp();
        addr1 = addr_createString(temp/*, tree->scope*/);
        addr2 = aux;
        cGen(p1);
        quad_insert(opVEC, addr1, addr2, aux);
        var = addr2;
        offset = aux;
        aux = addr1;
      }
      else {
       // printf("não entrou no vec \n");
        temp = newTemp();
        addr1 = addr_createString(temp/*, tree->scope*/);
        quad_insert(opLOAD, addr1, aux, empty);
        var = aux;
        offset = empty;
        aux = addr1;
      }
      if (TraceCode)  emitComment("<- Id");
      break;

    case TypeK:
     p1 = tree->child[0];
     cGen(p1);
    break;

    case FunDeclK:
      //printf("FundeclK: %s \n",tree->attr.name);
      if (TraceCode) emitComment("-> Fun");
      // ver se é a main
      if (strcmp(tree->attr.name, "main") == 0)
        mainLocation = location;
      if ((strcmp(tree->attr.name, "input") != 0) && (strcmp(tree->attr.name, "output") != 0)) {
        //printf("é diferente \n");
        quad_insert(opFUN, addr_createString(tree->attr.name/*, tree->scope*/), empty, empty);
         // parametros
        p1 = tree->child[0];
        cGen(p1);
        // declaracoes e expressoes
        p2 = tree->child[1];
        cGen(p2);
        quad_insert(opEND, addr_createString(tree->attr.name/*, tree->scope*/), empty, empty);
      }
      if (TraceCode) emitComment("<- Fun");
      break;
    
    case AtivK:
      //printf("AtivK \n");
      if (TraceCode) emitComment("-> Call");
      //Address a1 = addr_createIntConst(tree->params);
      // é um parametro
      nparams = tree->params;
      p1 = tree->child[0];
      //params_push(&nparams);
      while (p1 != NULL) {
        cGen(p1);
        quad_insert(opPARAM, aux, empty, empty);
        //printf("%s %d\n", tree->attr.name, tree->vet);
        nparams --;
        p1 = p1->sibling;
      }
      nparams = -1;
      temp = newTemp();
      aux = addr_createString(temp/*, tree->scope*/);
      quad_insert(opCALL, aux, addr_createString(tree->attr.name/*, tree->scope*/), addr_createIntConst(tree->params));
      
      if (TraceCode) emitComment("<- Call");
      break;
    
    case ParamK:
      //printf("ParamK \n");
      if (TraceCode) emitComment("-> Param");
      quad_insert(opARG, addr_createString(tree->attr.name/*, tree->scope*/), empty, empty); //addr_createString(tree->scope/*, tree->scope*/));
      if (TraceCode) emitComment("<- Param");
      break;
    
    case VarDeclK:
      //printf("VardeclK \n");
      if (TraceCode) emitComment("-> Var");
      //if (tree->vet != -1)
      //printf("tree size:%d \n",tree->size);
      if (tree->size != 0){
         //printf("tree size dif 0 \n");
        //quad_insert(DECLVEC, addr_createString(tree->attr.name/*, tree->scope*/), addr_createIntConst(tree->vet), addr_createString(tree->scope/*, tree->scope*/));
        quad_insert(opALLOC, addr_createString(tree->attr.name/*, tree->scope*/), addr_createIntConst(tree->size), empty);// addr_createString(tree->scope/*, tree->scope*/));
      }
      else 
        //printf("tree size igual a 0 \n");
        //quad_insert(DECLVAR, addr_createString(tree->attr.name/*, tree->scope*/), addr_createString(tree->scope/*, tree->scope*/), empty);
        quad_insert(opALLOC, addr_createString(tree->attr.name/*, tree->scope*/), addr_createIntConst(1),empty);// addr_createString(//tree->scope/*, tree->scope*/));
      if (TraceCode) emitComment("<- Var");
      break;

    case OpK:
     // printf("OpK: %d \n",tree->attr.op);
      if (TraceCode) emitComment("-> Op");
      p1 = tree->child[0];
      p2 = tree->child[1];
      cGen(p1);
      addr1 = aux;
      cGen(p2);
      addr2 = aux;
      temp = newTemp();
      aux = addr_createString(temp/*, tree->scope*/);
      switch (tree->attr.op) {
        case PLUS:
         // printf("plus \n");
          quad_insert(opADD, aux, addr1, addr2);
          break;
        case MINUS:
          //printf("dif\n");
          quad_insert(opSUB, aux, addr1, addr2); 
          break;
        case MULT:
          //printf("mult\n");
          quad_insert(opMULT, aux, addr1, addr2);
          break;
        case OVER:
          //printf("over\n");
          quad_insert(opDIV, aux, addr1, addr2);
          break;
        case LT:
          //printf("lt\n");
          quad_insert(opLT, aux, addr1, addr2);
          break;
        case LEQUAL:
          //printf("leq\n");
          quad_insert(opLEQUAL, aux, addr1, addr2);
          break;
        case GT:
         //printf("gt\n");
         quad_insert(opGT, aux, addr1, addr2); 
          break;
        case GREQUAL:
          //printf("greq\n");
          quad_insert(opGREQUAL, aux, addr1, addr2);
          break;
        case EQEQ:
          //printf("equal\n");
          quad_insert(opGREQUAL, aux, addr1, addr2);
          addr3 = aux;
          temp = newTemp();
          aux = addr_createString(temp/*, tree->scope*/);
          quad_insert(opLEQUAL, aux, addr1, addr2);
          addr1 = addr3;
          addr2 = aux;
          temp = newTemp();
          aux = addr_createString(temp/*, tree->scope*/);
          quad_insert(opAND, aux, addr1, addr2);
          break;
          case DIF:
          //printf("dif\n");
          quad_insert(opGT, aux, addr1, addr2);
          addr3 = aux;
          temp = newTemp();
          aux = addr_createString(temp/*, tree->scope*/);
          quad_insert(opLT, aux, addr1, addr2);
          addr1 = addr3;
          addr2 = aux;
          temp = newTemp();
          aux = addr_createString(temp/*, tree->scope*/);
          quad_insert(opOR, aux, addr1, addr2);
          break;
        default:
          emitComment("BUG: Unknown operator");
          break;
      }
      if (TraceCode)  emitComment("<- Op");
      break;

    default:
      break;
  }
} /* genExp */

/* Procedure cGen recursively generates code by
 * tree traversal
 */
static void cGen( TreeNode * tree)
{ 
  if(tree!=NULL)
 { 
  switch (tree->nodekind) {
      case StmtK:
        genStmt(tree);
        break;
      case ExpK: 
        genExp(tree);
        break;
      default:
        break;
    }
    if (nparams == -1) {
      cGen(tree->sibling);
    }
    else {
      if(nparams == 0) {
        cGen(tree->sibling);
      }
    }
  }
}

void printCode() {
 // printf("entrou no print \n");
  QuadList q = head;
  Address a1, a2, a3;
  while (q != NULL) {
    a1 = q->quad.addr1;
    a2 = q->quad.addr2;
    a3 = q->quad.addr3;
    printf("(%s, ", OpKindNames[q->quad.op]);
    switch (a1.kind) {
      case Empty:
        printf("-");
        break;
      case IntConst:
        printf("%d", a1.contents.val);
        break;
      case String:
        //printf("%s.%s", a1.contents.var.scope, a1.contents.var.name);
        printf("%s", a1.contents.var.name);
        break;
      default:
      break;
    }
    printf(", ");
    switch (a2.kind) {
      case Empty:
        printf("-");
        break;
      case IntConst:
        printf("%d", a2.contents.val);
        break;
      case String:
        //printf("%s.%s", a2.contents.var.scope, a2.contents.var.name);
        printf("%s", a2.contents.var.name);
        break;
      default:
        break;
    }
    printf(", ");
    switch (a3.kind) {
      case Empty:
        printf("-");
        break;
      case IntConst:
        printf("%d", a3.contents.val);
        break;
      case String:
        //printf("%s.%s", a3.contents.var.scope, a3.contents.var.name);
        printf("%s", a3.contents.var.name);
        break;
      default:
        break;
    }
    printf(")\n");
    q = q->next;
  }
}

/**********************************************/
/* the primary function of the code generator */
/**********************************************/
/* Procedure codeGen generates code to a code
 * file by traversal of the syntax tree. The
 * second parameter (codefile) is the file name
 * of the code file, and is used to print the
 * file name as a comment in the code file
 */

void codeGen(TreeNode * syntaxTree, char * codefile)
{  char * s = malloc(strlen(codefile)+7);
   strcpy(s,"File: ");
   strcat(s,codefile);
   emitComment("\nC- Intermediate Code");
   emitComment(s);
   empty = addr_createEmpty();
   //printf("Entrou \n");
   //printTree(syntaxTree);
   cGen(syntaxTree);
   quad_insert(opHLT, empty, empty, empty);
   printCode();
   /* finish */
   emitComment("End of execution");
}

QuadList getIntermediate() {
    return head; 
}