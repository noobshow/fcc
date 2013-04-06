#include "../inc/type.h"

#include "../inc/debug.h"
#include "../inc/sym.h"

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

static type* typeCreate ();

/*:::: TYPE CTORS/DTOR ::::*/

static type* typeCreate (typeTag tag) {
    type* DT = malloc(sizeof(type));
    DT->tag = tag;
    DT->basic = 0;

    DT->base = 0;
    DT->array = 0;

    DT->returnType = 0;
    DT->paramTypes = 0;
    DT->params = 0;

    return DT;
}

type* typeCreateBasic (struct sym* basic) {
    type* DT = typeCreate(typeBasic);
    DT->basic = basic;
    return DT;
}

type* typeCreatePtr (type* base) {
    type* DT = typeCreate(typePtr);
    DT->base = base;
    return DT;
}

type* typeCreateArray (type* base, int size) {
    type* DT = typeCreate(typeArray);
    DT->base = base;
    DT->array = size;
    return DT;
}

type* typeCreateFunction (type* returnType, type** paramTypes, int params) {
    type* DT = typeCreate(typeFunction);
    DT->returnType = returnType;
    DT->paramTypes = paramTypes;
    DT->params = params;
    return DT;
}

type* typeCreateInvalid () {
    return typeCreate(typeInvalid);
}

void typeDestroy (type* DT) {
    debugAssert("typeDestroy", "null parameter", DT != 0);

    if (typeIsBasic(DT) || typeIsInvalid(DT))
        ;

    else if (typeIsPtr(DT) || typeIsArray(DT))
        typeDestroy(DT->base);

    else if (typeIsFunction(DT)) {
        typeDestroy(DT->returnType);

        for (int i = 0; i < DT->params; i++)
            typeDestroy(DT->paramTypes[i]);

        free(DT->paramTypes);
    }

    free(DT);
}

type* typeDeepDuplicate (const type* DT) {
    debugAssert("typeDeepDuplicate", "null parameter", DT != 0);

    if (typeIsInvalid(DT))
        return typeCreateInvalid();

    else if (typeIsBasic(DT))
        return typeCreateBasic(DT->basic);

    else if (typeIsPtr(DT))
        return typeCreatePtr(typeDeepDuplicate(DT->base));

    else if (typeIsArray(DT))
        return typeCreateArray(typeDeepDuplicate(DT->base), DT->array);

    else if (typeIsFunction(DT)) {
        type** paramTypes = calloc(DT->params, sizeof(type*));

        for (int i = 0; i < DT->params; i++)
            paramTypes[i] = typeDeepDuplicate(DT->paramTypes[i]);

        return typeCreateFunction(typeDeepDuplicate(DT->returnType), paramTypes, DT->params);

    } else {
        debugErrorUnhandled("typeDeepDuplicate", "type tag", typeTagGetStr(DT->tag));
        return typeCreateInvalid();
    }
}

/*:::: TYPE DERIVATION ::::*/

type* typeDeriveFrom (const type* DT) {
    type* New = typeDeepDuplicate(DT);
    return New;
}

type* typeDeriveFromTwo (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveFromTwo", "type compatibility", typeIsCompatible(L, R));
        return typeDeriveFrom(L);
    }
}

type* typeDeriveUnified (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveUnified", "type compatibility", typeIsCompatible(L, R));

        if (typeIsEqual(L, R))
            return typeDeepDuplicate(L); //== R

        else
            return typeDeriveFromTwo(L, R);
    }
}

type* typeDeriveBase (const type* DT) {
    if (typeIsInvalid(DT))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveBase", "base", typeIsPtr(DT) || typeIsArray(DT));
        return typeDeepDuplicate(DT->base);
    }
}

type* typeDerivePtr (const type* base) {
    return typeCreatePtr(typeDeepDuplicate(base));
}

type* typeDeriveArray (const type* base, int size) {
    return typeCreateArray(typeDeepDuplicate(base), size);
}

type* typeDeriveReturn (const type* DT) {
    if (typeIsInvalid(DT))
        return typeCreateInvalid();

    else {
        debugAssert("typeDeriveReturn", "callable param", typeIsCallable(DT));

        if (typeIsPtr(DT))
            return typeDeriveReturn(DT->base);

        else
            return typeDeepDuplicate(DT->returnType);
    }
}

/*:::: TYPE CLASSIFICATION ::::*/

bool typeIsBasic (const type* DT) {
    return DT->tag == typeBasic || typeIsInvalid(DT);
}

bool typeIsPtr (const type* DT) {
    return DT->tag == typePtr || typeIsInvalid(DT);
}

bool typeIsArray (const type* DT) {
    return DT->tag == typeArray || typeIsInvalid(DT);
}

bool typeIsFunction (const type* DT) {
    return DT->tag == typeFunction || typeIsInvalid(DT);
}

bool typeIsInvalid (const type* DT) {
    return DT->tag == typeInvalid;
}

bool typeIsVoid (const type* DT) {
    /*Is it a built in type of size zero (void)*/
    return    (   (DT->tag == typeBasic && DT->basic->tag == symType)
               && typeGetSize(DT) == 0)
           || typeIsInvalid(DT);
}

bool typeIsRecord (const type* DT) {
    return    (DT->tag == typeBasic && DT->basic->tag == symStruct)
           || typeIsInvalid(DT);
}

bool typeIsCallable (const type* DT) {
    return    (   typeIsFunction(DT)
               || (DT->tag == typePtr && typeIsFunction(DT->base)))
           || typeIsInvalid(DT);
}

bool typeIsNumeric (const type* DT) {
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeNumeric))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsOrdinal (const type* DT) {
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeOrdinal))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsEquality (const type* DT) {
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeEquality))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsAssignment (const type* DT) {
    return    (DT->tag == typeBasic && (DT->basic->typeMask & typeAssignment))
           || typeIsPtr(DT) || typeIsInvalid(DT);
}

bool typeIsCondition (const type* DT) {
    return (DT->tag == typeBasic && (DT->basic->typeMask & typeCondition)) ||
            typeIsPtr(DT) || typeIsInvalid(DT);
}

/*:::: TYPE COMPARISON ::::*/

bool typeIsCompatible (const type* DT, const type* Model) {
    if (typeIsInvalid(DT) || typeIsInvalid(Model))
        return true;

    /*If function requested, match params and return*/
    else if (typeIsFunction(Model)) {
        if (Model->params != DT->params)
            return false;

        for (int i = 0; i < Model->params; i++)
            if (!typeIsEqual(DT->paramTypes[i],
                             Model->paramTypes[i]))
                return false;

        return typeIsEqual(DT->returnType,
                           Model->returnType);

    /*If pointer requested, allow pointers and arrays of matching type
      and basic numeric types
      If void*, accept all arrays and pointers*/
    } else if (typeIsPtr(Model))
        return    (   (typeIsPtr(DT) || typeIsArray(DT))
                   && (   typeIsVoid(Model->base)
                       || typeIsCompatible(DT->base, Model->base)))
               || (typeIsBasic(DT) && (DT->basic->typeMask & typeNumeric));

    /*If array requested, accept only arrays of matching size and type*/
    else if (typeIsArray(Model))
        return    typeIsArray(DT)
               && ((DT->array == Model->array) || Model->array == -1)
               && typeIsCompatible(DT->base, Model->base);

    /*Basic type*/
    else {
        if (typeIsPtr(DT))
            return Model->basic->typeMask & typeNumeric;

        else
            return !typeIsArray(DT) && DT->basic == Model->basic
    }
}

bool typeIsEqual (const type* L, const type* R) {
    if (typeIsInvalid(L) || typeIsInvalid(R))
        return true;

    else if (L->tag != R->tag)
        return false;

    else if (typeIsFunction(L))
        return typeIsCompatible(L, R);

    else if (typeIsPtr(L))
        return typeIsEqual(L->base, R->base);

    else if (typeIsArray(L))
        return    L->array == R->array
               && typeIsEqual(L->base, R->base);

    else /*(typeIsBasic(L))*/
        return L->basic == R->basic;
}

/*:::: MISC INTERFACES ::::*/

const char* typeTagGetStr (typeTag tag) {
    if (tag == typeBasic)
        return "typeBasic";
    else if (tag == typePtr)
        return "typePtr";
    else if (tag == typeArray)
        return "typeArray";
    else if (tag == typeFunction)
        return "typeFunction";
    else if (tag == typeInvalid)
        return "typeInvalid";

    else {
        char* str = malloc(logi(tag, 10)+2);
        sprintf(str, "%d", tag);
        debugErrorUnhandled("typeTagGetStr", "symbol tag", str);
        free(str);
        return "unhandled";
    }
}

int typeGetSize (const type* DT) {
    if (typeIsInvalid(DT))
        return 0;

    else if (typeIsArray(DT))
        return DT->array * typeGetSize(DT->base);

    else if (typeIsPtr(DT) || typeIsFunction(DT))
        return 8; //yummy magic number

    else /*if (typeIsBasic(DT))*/
        return DT->basic->size;
}

char* typeToStr (const type* DT, const char* embedded) {
    /*Basic type or invalid*/
    if (typeIsInvalid(DT) || typeIsBasic(DT)) {
        char* basicStr = typeIsInvalid(DT)
                            ? "<invalid>"
                            : DT->basic->ident;

        if (embedded[0] == 0)
            return strdup(basicStr);

        else {
            char* ret = malloc(strlen(embedded) +
                               strlen(basicStr)+2);

            sprintf(ret, "%s %s", basicStr, embedded);
            return ret;
        }

    /*Function*/
    } else if (typeIsFunction(DT)) {
        /*Get the param string that goes inside the parens*/

        char* params = 0;

        if (DT->params != 0) {
            char* paramStrs[DT->params];
            int length = 1;

            /*Get strings for all the params and count total string length*/
            for (int i = 0; i < DT->params; i++) {
                paramStrs[i] = typeToStr(DT->paramTypes[i], "");
                length += strlen(paramStrs[i])+2;
            }

            /*Cat them together*/

            params = malloc(length+1);

            int charno = 0;

            for (int i = 0; i < DT->params-1; i++)
                charno += sprintf(params+charno, "%s, ", paramStrs[i]);

            /*Cat the final one, sans the delimiting comma*/
            sprintf(params+charno, "%s", paramStrs[DT->params-1]);

        } else
            params = strdup("void");

        /* */

        char* format = malloc(strlen(embedded) +
                              strlen(params)+5);
        sprintf(format, "(%s)(%s)", embedded, params);
        free(params);
        char* ret = typeToStr(DT->returnType, format);
        free(format);
        return ret;

    /*Array or Ptr*/
    } else {
        char* format = 0;

        if (typeIsPtr(DT)) {
            format = malloc(strlen(embedded)+2);
            sprintf(format, "*%s", embedded);

        } else /*if (typeIsArray(DT))*/ {
            format = malloc(strlen(embedded) +
                            logi(DT->array, 10)+4);

            if (DT->array == -1)
                sprintf(format, "%s[]", embedded);

            else
                sprintf(format, "%s[%d]", embedded, DT->array);
        }

        char* ret = typeToStr(DT->base, format);
        free(format);
        return ret;
    }
}
