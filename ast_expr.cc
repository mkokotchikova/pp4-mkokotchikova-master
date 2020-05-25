/* File: ast_expr.cc
 * -----------------
 * Implementation of expression node classes.
 */
#include "ast_expr.h"
#include "ast_type.h"
#include "ast_decl.h"
#include <string.h>

#include "errors.h"


IntConstant::IntConstant(yyltype loc, int val) : Expr(loc) {
    value = val;
}

DoubleConstant::DoubleConstant(yyltype loc, double val) : Expr(loc) {
    value = val;
}

BoolConstant::BoolConstant(yyltype loc, bool val) : Expr(loc) {
    value = val;
}

StringConstant::StringConstant(yyltype loc, const char *val) : Expr(loc) {
    Assert(val != NULL);
    value = strdup(val);
}

Operator::Operator(yyltype loc, const char *tok) : Node(loc) {
    Assert(tok != NULL);
    strncpy(tokenString, tok, sizeof(tokenString));
}
CompoundExpr::CompoundExpr(Expr *l, Operator *o, Expr *r) 
  : Expr(Join(l->GetLocation(), r->GetLocation())) {
    Assert(l != NULL && o != NULL && r != NULL);
    (op=o)->SetParent(this);
    (left=l)->SetParent(this); 
    (right=r)->SetParent(this);
}

CompoundExpr::CompoundExpr(Operator *o, Expr *r) 
  : Expr(Join(o->GetLocation(), r->GetLocation())) {
    Assert(o != NULL && r != NULL);
    left = NULL; 
    (op=o)->SetParent(this);
    (right=r)->SetParent(this);
}
   

// Эта конфигурация добавляется для постфиксной формы записи
CompoundExpr::CompoundExpr(Expr *l, Operator *o) 
  : Expr(Join(l->GetLocation(), o->GetLocation())) {
    Assert(o != NULL && l != NULL);
    (left=l)->SetParent(this) ; 
    (op=o)->SetParent(this);
    right=NULL;
} 

void CompoundExpr::Check() {
  Type* aux;
  aux = CompoundExpr::GetType();
  return;
}

Type* CompoundExpr::GetType(){
    Type* typeLeft;
    Type* typeRight;
    typeLeft = CompoundExpr::GetLeft();
    typeRight = CompoundExpr::GetRight(); 
    //Если есть ошибки типов, то эта ошибка должна быть сообщена заранее
    if ((typeLeft==Type::errorType) || (typeRight==Type::errorType)){
        return Type::errorType;
    }
    //Просмотр всех операций
    if ((typeLeft != Type::nullType) &&(typeRight!= Type::nullType)){
        if(typeLeft!=typeRight){
            ReportError::IncompatibleOperands(op, typeLeft, typeRight);
            return Type::errorType;
        }
    }
    return typeRight;
    
}

Type* CompoundExpr::GetLeft(){
    
    if (left == NULL){
        return(Type::nullType);
    }
    return(left->GetType());

}
Type* CompoundExpr::GetRight(){
    
    if (right == NULL){
        return(Type::nullType);
    }
    return(right->GetType());

}

void ArithmeticExpr::Check(){
      return;
}

Type* ArithmeticExpr::GetType(){
    //Получить типы слева направо
    Type* typeLeft;
    Type* typeRight;
    
    typeLeft = CompoundExpr::GetLeft();
    typeRight = CompoundExpr::GetRight(); 

    //Если есть ошибки, их надо сообщить ранее
    if ((typeLeft==Type::errorType) || (typeRight==Type::errorType)){
        return Type::errorType;
    }

    //В арифметических выражениях левая часть может быть пустой например число со знаком -7 */
    if ((typeLeft != Type::nullType)){
        if(typeLeft->IsEquivalentTo(typeRight)==false){
            //Если есть типы которые не совпадают, сообщить об ошибке
	    ReportError::IncompatibleOperands(op, typeLeft, typeRight);
            return Type::errorType;
        }
    }
    return typeRight;
    
}

void AssignExpr::Check(){
    Type* aux;
    aux = AssignExpr::GetType();
    return;
}


Type* AssignExpr::GetType(){
    Type* typeLeft;
    Type* typeRight;
    typeLeft = AssignExpr::GetLeft();
    typeRight = AssignExpr::GetRight(); 
    if ((typeLeft==Type::errorType) || (typeRight==Type::errorType)){
        return Type::errorType;
    }
    if ((typeLeft != Type::nullType) &&(typeRight!= Type::nullType)){
        if(typeLeft->IsEquivalentTo(typeRight)==false){
            ReportError::IncompatibleOperands(op, typeLeft, typeRight);
            return Type::errorType;
        }
    }
    return typeLeft;
    
}

Type* AssignExpr::GetLeft(){
    if (left == NULL){
        return(Type::nullType);
    }
    Type* leftType = left->GetType();

    return(leftType);

}


Type* AssignExpr::GetRight(){
    if (right == NULL){
        return(Type::nullType);
    }
    return(right->GetType());

}

ArrayAccess::ArrayAccess(yyltype loc, Expr *b, Expr *s) : LValue(loc) {
    (base=b)->SetParent(this); 
    (subscript=s)->SetParent(this);
}

void ArrayAccess::Check(){
    //Проверить что основание типа массив
    Type* basetype;
    basetype = base->GetType();
    if(dynamic_cast<ArrayType*>(basetype)==NULL){
        ReportError::BracketsOnNonArray(base);
    }

    //Проверить что индекс массива это целый тип
    Type* stype = subscript->GetType();
    if(stype!=Type::intType){
        ReportError::SubscriptNotInteger(subscript);
    }
    
}

Type* ArrayAccess::GetType(){
    //Проверить, что это тип массив и изъять элемент
    ArrayAccess::Check();
    Type* basetype;
    basetype = base->GetType();
    if(dynamic_cast<ArrayType*>(basetype)==NULL){
        return Type::errorType;
    }
    ArrayType* arrt = dynamic_cast<ArrayType*>(basetype);
    return arrt->GetElemType();
}


     
FieldAccess::FieldAccess(Expr *b, Identifier *f) 
  : LValue(b? Join(b->GetLocation(), f->GetLocation()) : *f->GetLocation()) {
    Assert(f != NULL); // b can be be NULL (just means no explicit base)
    base = b; 
    if (base) base->SetParent(this); 
    (field=f)->SetParent(this);
}



Type* FieldAccess::GetType(){
    //Найти объявления поля, преобразовать переменную и тип
    
    if (base == NULL){
        //Если переменная отсутсвует внутри класса
        VarDecl *vardecl = dynamic_cast<VarDecl*>(parent->FindDecl(field));
        if (vardecl ==NULL){
            //Проверить, заявлен ли тип
             ReportError::IdentifierNotDeclared(field, LookingForVariable);
             return Type::errorType;
     }
     
     Type* vartype = vardecl->GetDeclaredType();
     return vartype;
    }
    //Проверить, что основа есть названный тип не примитивный и не массив
    if(dynamic_cast<NamedType*>(base->GetType())==NULL){
        ReportError::FieldNotFoundInBase(field, base->GetType());
        return Type::errorType;
    }else{
        //Изменить названный тип и найти соответвенный класс
        Identifier *id = dynamic_cast<NamedType*>(base->GetType())->GetId();
        ClassDecl *classdecl  = dynamic_cast<ClassDecl*>(parent->FindDecl(id));
        if(classdecl==NULL){
            ReportError::IdentifierNotDeclared(id, LookingForClass);
            return Type::errorType;
        }
        Scope *classScope = classdecl->PrepareScope();
        if(classScope->Lookup(field)==NULL){
            ReportError::FieldNotFoundInBase(field, base->GetType());
            return Type::errorType;
        }
        
        }
    //Проверить что класс искомая область
    if (IsClassScope()==false){
        ReportError::InaccessibleField(field, base->GetType());
        return Type::errorType;
    }
    return Type::errorType;
}

void FieldAccess::Check(){
    FieldAccess::GetType();
}



Call::Call(yyltype loc, Expr *b, Identifier *f, List<Expr*> *a) : Expr(loc)  {
    Assert(f != NULL && a != NULL); // b can be be NULL (just means no explicit base)
    base = b;
    if (base) base->SetParent(this);
    (field=f)->SetParent(this);
    (actuals=a)->SetParentAll(this);
}

void Call::Check(){
    actuals->CheckAll();
    if (base==NULL){
        //Если она не в классе
        FnDecl *fndecl = dynamic_cast<FnDecl*>(parent->FindDecl(field));
            if (fndecl == NULL){
                //Проверка на существования объявления
                ReportError::IdentifierNotDeclared(field, LookingForFunction);
                return;
        }
        //Проверка что формально и на самом деле одинковые размеры
        List<VarDecl*> formals = fndecl->GetFormals();
        if(actuals->NumElements()!=formals.NumElements()){
            ReportError::NumArgsMismatch(field, formals.NumElements(), actuals->NumElements());
            return;
        }
        return;
    }
    //Проверка что база имеет перечисленный тип не примитивный и не массив
    if(dynamic_cast<NamedType*>(base->GetType())==NULL){
        ReportError::FieldNotFoundInBase(field, base->GetType());
        return;
    }else{
        //Преобразовать объявления названного типа и искать его для опреденения класса
        Identifier *id = dynamic_cast<NamedType*>(base->GetType())->GetId();
        ClassDecl *classdecl  = dynamic_cast<ClassDecl*>(parent->FindDecl(id));
        if(classdecl==NULL){
            ReportError::IdentifierNotDeclared(id, LookingForClass);
            return;
        }
        Scope *classScope = classdecl->PrepareScope();
        if(classScope->Lookup(field)==NULL){
            ReportError::FieldNotFoundInBase(field, base->GetType());
            return;
        }
        
        
    }


};

NewExpr::NewExpr(yyltype loc, NamedType *c) : Expr(loc) { 
  Assert(c != NULL);
  (cType=c)->SetParent(this);
}


NewArrayExpr::NewArrayExpr(yyltype loc, Expr *sz, Type *et) : Expr(loc) {
    Assert(sz != NULL && et != NULL);
    (size=sz)->SetParent(this); 
    (elemType=et)->SetParent(this);
    currloc=loc; // Добавим местоположение для новой переменнй в массиве
}

       
void NewArrayExpr::Check(){
    //Проверить, что размер массива целое число
    Type* sztype = size->GetType();
    if(sztype!=Type::intType){
        ReportError::NewArraySizeNotInteger(size);
    }

}

Type* NewArrayExpr::GetType(){
    NewArrayExpr::Check();
    return new ArrayType(currloc, elemType);
}

void This::Check(){
    //Проверить скоп(область)
    if (IsClassScope()==false){
        ReportError::ThisOutsideClassScope(this);
    }
}
Type* This::GetType(){
    This::Check();
    return Type::errorType;
}
