/* File: ast.cc
 * ------------
 */

#include "ast.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h> // strdup
#include <stdio.h>  // printf
#include "errors.h"
#include "scope.h"

Node::Node(yyltype loc) {
  
    location = new yyltype(loc);
    parent = NULL;
    nodeScope = NULL;
}

Node::Node() {

    location = NULL;
    parent = NULL;
    nodeScope = NULL;
}

Decl *Node::FindDecl(Identifier *idToFind, lookup l) {
    Decl *mine;
    if (!nodeScope) PrepareScope();
    if (nodeScope && (mine = nodeScope->Lookup(idToFind)))
        return mine;
    if (l == kDeep && parent)
        return parent->FindDecl(idToFind, l);
    return NULL;
}

Identifier::Identifier(yyltype loc, const char *n) : Node(loc) {
    name = strdup(n);
    cached = NULL;
}


/*Проверяем, что находиться в скопе классов*/
bool Node::IsClassScope(lookup l) {
    //printf("Class lookup\n");
    if (!nodeScope) PrepareScope();
    if (nodeScope && (dynamic_cast<ClassDecl*>(parent)!=NULL))
        return true;
    if (l == kDeep && parent)
        return parent->IsClassScope(l);
    return false;
}

