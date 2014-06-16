// ============================================================================
//        __
//   \\__/ o\    (C) 2012-2014  Robert Finch, Stratford
//    \  __ /    All rights reserved.
//     \/_//     robfinch<remove>@finitron.ca
//       ||
//
// C64 - 'C' derived language compiler
//  - 64 bit CPU
//
// This source file is free software: you can redistribute it and/or modify 
// it under the terms of the GNU Lesser General Public License as published 
// by the Free Software Foundation, either version 3 of the License, or     
// (at your option) any later version.                                      
//                                                                          
// This source file is distributed in the hope that it will be useful,      
// but WITHOUT ANY WARRANTY; without even the implied warranty of           
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            
// GNU General Public License for more details.                             
//                                                                          
// You should have received a copy of the GNU General Public License        
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    
//                                                                          
// ============================================================================
//
#include        <stdio.h>
#include        "c.h"
#include        "expr.h"
#include "Statement.h"
#include        "gen.h"
#include        "cglbdec.h"

#define EXPR_DEBUG
static unsigned char sizeof_flag = 0;

/*
 *	68000 C compiler
 *
 *	Copyright 1984, 1985, 1986 Matthew Brandt.
 *  all commercial rights reserved.
 *
 *	This compiler is intended as an instructive tool for personal use. Any
 *	use for profit without the written consent of the author is prohibited.
 *
 *	This compiler may be distributed freely for non-commercial use as long
 *	as this notice stays intact. Please forward any enhancements or questions
 *	to:
 *
 *		Matthew Brandt
 *		Box 920337
 *		Norcross, Ga 30092
 */

TYP             stdint = { bt_long, bt_long, 0, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stduint = { bt_long, bt_long, 0, TRUE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdlong = { bt_long, bt_long, 0, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdulong = { bt_long, bt_long, 0, TRUE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0 };
TYP             stdshort = { bt_short, bt_short, 0, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0 };
TYP             stdushort = { bt_short, bt_short, 0, TRUE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0 };
TYP             stdchar = {bt_char, bt_char, 0, FALSE, FALSE, FALSE, FALSE, 0,0,2, {0, 0}, 0, 0 };
TYP             stduchar = {bt_char, bt_char, 0, TRUE, FALSE, FALSE, FALSE, 0,0,2, {0, 0}, 0, 0 };
TYP             stdbyte = {bt_byte, bt_byte, 0, FALSE, FALSE, FALSE, FALSE, 0,0,1, {0, 0}, 0, 0 };
TYP             stdubyte = {bt_byte, bt_byte, 0, TRUE, FALSE, FALSE, FALSE, 0,0,1, {0, 0}, 0, 0 };
TYP             stdstring = {bt_pointer, bt_pointer, 1, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, &stdchar, 0};
TYP				stddbl = {bt_double, bt_double, 0, FALSE, FALSE, FALSE, FALSE, 0,0,8, {0, 0}, 0, 0};
TYP				stdflt = {bt_float, bt_float, 0, FALSE, FALSE, FALSE, FALSE, 0,0,4, {0, 0}, 0, 0};
TYP             stdfunc = {bt_func, bt_func, 1, FALSE, FALSE, FALSE, FALSE, 0,0,0, {0, 0}, &stdint, 0};
extern TYP      *head;          /* shared with ParseSpecifier */
extern TYP	*tail;

/*
 *      expression evaluation
 *
 *      this set of routines builds a parse tree for an expression.
 *      no code is generated for the expressions during the build,
 *      this is the job of the codegen module. for most purposes
 *      expression() is the routine to call. it will allow all of
 *      the C operators. for the case where the comma operator is
 *      not valid (function parameters for instance) call NonCommaExpression().
 *
 *      each of the routines returns a pointer to a describing type
 *      structure. each routine also takes one parameter which is a
 *      pointer to an expression node by reference (address of pointer).
 *      the completed expression is returned in this pointer. all
 *      routines return either a pointer to a valid type or NULL if
 *      the hierarchy of the next operator is too low or the next
 *      symbol is not part of an expression.
 */

TYP     *expression();  /* forward ParseSpecifieraration */
TYP     *NonCommaExpression();      /* forward ParseSpecifieraration */
TYP     *ParseUnaryExpression();       /* forward ParseSpecifieraration */

/*
 *      build an expression node with a node type of nt and values
 *      v1 and v2.
 */
ENODE *makenode(int nt, ENODE *v1, ENODE *v2)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->p[0] = v1;
	ep->p[1] = v2;
    return ep;
}

ENODE *makesnode(int nt, char *v1, __int64 i)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->sp = v1;
	ep->i = i;
    return ep;
}

ENODE *makenodei(int nt, ENODE *v1, __int64 i)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = nt;
    ep->constflag = FALSE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
	ep->i = i;
	ep->p[0] = v1;
	ep->p[1] = NULL;
    return ep;
}

ENODE *makeinode(int nt, __int64 v1)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = nt;
    ep->constflag = TRUE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
    ep->i = v1;
    return ep;
}

ENODE *makefnode(int nt, double v1)
{
	ENODE *ep;
    ep = (ENODE *)xalloc(sizeof(ENODE));
    ep->nodetype = nt;
    ep->constflag = TRUE;
	ep->isUnsigned = FALSE;
	ep->etype = bt_void;
	ep->esize = -1;
    ep->f = v1;
    return ep;
}

void PromoteConstFlag(ENODE *ep)
{
	ep->constflag = ep->p[0]->constflag && ep->p[1]->constflag;
}

/*
 *      build the proper dereference operation for a node using the
 *      type pointer tp.
 */
TYP *deref(ENODE **node, TYP *tp)
{
	switch( tp->type ) {
		case bt_byte:
			if (tp->isUnsigned) {
				*node = makenode(en_ub_ref,*node,NULL);
				(*node)->isUnsigned = TRUE;
			}
			else
				*node = makenode(en_b_ref,*node,NULL);
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            tp = &stdint;
            break;
		case bt_char:
        case bt_enum:
			if (tp->isUnsigned) {
				*node = makenode(en_uc_ref,*node,NULL);
				(*node)->isUnsigned = TRUE;
			}
			else
				*node = makenode(en_c_ref,*node,NULL);
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            tp = &stdchar;
            break;
        case bt_short:
			if (tp->isUnsigned) {
				*node = makenode(en_uh_ref,*node,NULL);
				(*node)->esize = tp->size;
				(*node)->etype = tp->type;
				(*node)->isUnsigned = TRUE;
				tp = &stduint;
			}
			else {
				*node = makenode(en_h_ref,*node,NULL);
				(*node)->esize = tp->size;
				(*node)->etype = tp->type;
				tp = &stdint;
			}
            break;
		case bt_long:
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
			if (tp->isUnsigned) {
				(*node)->isUnsigned = TRUE;
				*node = makenode(en_uw_ref,*node,NULL);
			}
			else {
				*node = makenode(en_w_ref,*node,NULL);
			}
            break;
		case bt_pointer:
		case bt_unsigned:
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            *node = makenode(en_uw_ref,*node,NULL);
			(*node)->isUnsigned = TRUE;
            break;
        case bt_double:
            *node = makenode(en_dbl_ref,*node,NULL);
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            tp = &stddbl;
            break;
        case bt_float:
            *node = makenode(en_flt_ref,*node,NULL);
			(*node)->esize = tp->size;
			(*node)->etype = tp->type;
            tp = &stdflt;
            break;
		case bt_bitfield:
			if (tp->isUnsigned){
				if (tp->size==1)
					*node = makenode(en_ubfieldref, *node, NULL);
				else if (tp->size==2)
					*node = makenode(en_ucfieldref, *node, NULL);
				else if (tp->size==4)
					*node = makenode(en_uhfieldref, *node, NULL);
				else
					*node = makenode(en_wfieldref, *node, NULL);
				(*node)->isUnsigned = TRUE;
			}
			else {
				if (tp->size==1)
					*node = makenode(en_bfieldref, *node, NULL);
				else if (tp->size==2)
					*node = makenode(en_cfieldref, *node, NULL);
				else if (tp->size==4)
					*node = makenode(en_hfieldref, *node, NULL);
				else
					*node = makenode(en_wfieldref, *node, NULL);
			}
			(*node)->bit_width = tp->bit_width;
			(*node)->bit_offset = tp->bit_offset;
			/*
			* maybe it should be 'unsigned'
			*/
			(*node)->etype = stdint.type;
			(*node)->esize = tp->size;
			tp = &stdint;
			break;
		//case bt_func:
		//case bt_ifunc:
		//	(*node)->esize = tp->size;
		//	(*node)->etype = tp->type;
		//	(*node)->isUnsigned = TRUE;
		//	*node = makenode(en_uw_ref,*node,NULL);
  //          break;
		//case bt_struct:
		//case bt_union:
		//	(*node)->esize = tp->size;
		//	(*node)->etype = tp->type;
  //          *node = makenode(en_struct_ref,*node,NULL);
		//	(*node)->isUnsigned = TRUE;
  //          break;
		default:
			error(ERR_DEREF);
			break;
    }
	(*node)->isVolatile = tp->isVolatile;
    return tp;
}

/*
* dereference the node if val_flag is zero. If val_flag is non_zero and
* tp->type is bt_pointer (array reference) set the size field to the
* pointer size if this code is not executed on behalf of a sizeof
* operator
*/
TYP *CondDeref(ENODE *node, TYP *tp)
{
    TYP *tp1;

    if (tp->val_flag == 0)
		return deref(node, tp);
    if (tp->type == bt_pointer && sizeof_flag == 0) {
		tp1 = tp->btp;
		tp = maketype(bt_pointer, 8);
		tp->btp = tp1;
    }
    return tp;
}


/*
 *      nameref will build an expression tree that references an
 *      identifier. if the identifier is not in the global or
 *      local symbol table then a look-ahead to the next character
 *      is done and if it indicates a function call the identifier
 *      is coerced to an external function name. non-value references
 *      generate an additional level of indirection.
 */
TYP *nameref(ENODE **node)
{
	SYM             *sp;
    TYP             *tp;
	char stnm[200];
    sp = gsearch(lastid);
    if( sp == NULL ) {
        while( isspace(lastch) )
            getch();
        if( lastch == '(') {
            ++global_flag;
            sp = allocSYM();
            sp->tp = &stdfunc;
            sp->name = litlate(lastid);
            sp->storage_class = sc_external;
            insert(sp,&gsyms);
            --global_flag;
            tp = &stdfunc;
            *node = makesnode(en_cnacon,sp->name,sp->value.i);
            (*node)->constflag = TRUE;
			if (sp->tp->isUnsigned)
				(*node)->isUnsigned = TRUE;
        }
        else {
            tp = NULL;
            error(ERR_UNDEFINED);
        }
    }
    else    {
            if( (tp = sp->tp) == NULL ) {
                error(ERR_UNDEFINED);
                return NULL;       /* guard against untyped entries */
            }
            switch( sp->storage_class ) {
                    case sc_static:
						if (sp->tp->type==bt_func || sp->tp->type==bt_ifunc) {
								//strcpy(stnm,GetNamespace());
								//strcat(stnm,"_");
								stnm[0] = '\0';
								strcat(stnm,sp->name);
								*node = makesnode(en_cnacon,litlate(stnm),sp->value.i);
								(*node)->constflag = TRUE;
								//*node = makesnode(en_nacon,sp->name);
								//(*node)->constflag = TRUE;
							}
							else {
								*node = makeinode(en_labcon,sp->value.i);
								(*node)->constflag = TRUE;
							}
							if (sp->tp->isUnsigned)
								(*node)->isUnsigned = TRUE;
                            break;
					case sc_thread:
							*node = makeinode(en_labcon,sp->value.i);
							(*node)->constflag = TRUE;
							if (sp->tp->isUnsigned)
								(*node)->isUnsigned = TRUE;
							break;
                    case sc_global:
                    case sc_external:
							if (sp->tp->type==bt_func || sp->tp->type==bt_ifunc)
	                            *node = makesnode(en_cnacon,sp->name,sp->value.i);
							else
	                            *node = makesnode(en_nacon,sp->name,sp->value.i);
                            (*node)->constflag = TRUE;
							(*node)->isUnsigned = sp->tp->isUnsigned;
                            break;
                    case sc_const:
							if (sp->tp->type==bt_float || sp->tp->type==bt_double)
								*node = makefnode(en_fcon,sp->value.f);
							else {
								*node = makeinode(en_icon,sp->value.i);
								if (sp->tp->isUnsigned)
									(*node)->isUnsigned = TRUE;
							}
                            (*node)->constflag = TRUE;
                            break;
                    default:        /* auto and any errors */
                            if( sp->storage_class != sc_auto)
                                    error(ERR_ILLCLASS);
							if (sp->tp->type==bt_float || sp->tp->type==bt_double)
								*node = makeinode(en_autofcon,sp->value.i);
							else {
								*node = makeinode(en_autocon,sp->value.i);
								if (sp->tp->isUnsigned)
									(*node)->isUnsigned = TRUE;
							}
                            break;
                    }
                    tp = CondDeref(node,tp);
            }
    NextToken();
    return tp;
}

/*
 *      ArgumentList will build a list of parameter expressions in
 *      a function call and return a pointer to the last expression
 *      parsed. since parameters are generally pushed from right
 *      to left we get just what we asked for...
 */
ENODE *ArgumentList()
{
	struct ENODE    *ep1, *ep2;
    ep1 = 0;
    while( lastst != closepa)
	{
        NonCommaExpression(&ep2);          /* evaluate a parameter */
        ep1 = makenode(en_void,ep2,ep1);
        if( lastst != comma)
            break;
        NextToken();
    }
    return ep1;
}

/*
 *      return 1 if st in set of [ kw_char, kw_short, kw_long, kw_int,
 *      kw_float, kw_double, kw_struct, kw_union ]
 */
static int IsIntrinsicType(int st)
{
	return  st == kw_byte || st==kw_char || st == kw_short || st == kw_int || st==kw_void ||
				st == kw_int16 || st == kw_int8 || st == kw_int32 || st == kw_int16 ||
                st == kw_long || st == kw_float || st == kw_double || st==kw_enum ||
                st == kw_struct || st == kw_union || st== kw_unsigned || st==kw_signed ||
				st == kw_const;
}

int IsBeginningOfTypecast(int st)
{
	SYM *sp;
	if (st==id) {
		sp = search(lastid,&gsyms[0]);
		if (sp)
			return sp->storage_class==sc_typedef;
		return FALSE;
	}
	else
		return IsIntrinsicType(st);
}

/*
 *      primary will parse a primary expression and set the node pointer
 *      returning the type of the expression parsed. primary expressions
 *      are any of:
 *                      id
 *                      constant
 *                      string
 *                      ( expression )
 *                      primary[ expression ]
 *                      primary.id
 *                      primary->id
 *                      primary( parameter list )
 */
TYP *ParsePrimaryExpression(ENODE **node)
{
	ENODE    *pnode, *qnode, *rnode, *snode, *rnode1, *pnode1, *qnode1, *qnode2;
	__int64 i ;
	int sza[10];
	int brcount;
        SYM             *sp;
        TYP             *tptr;

		brcount = 0;
		qnode1 = NULL;
		qnode2 = NULL;
        switch( lastst ) {
				case ellipsis:
                case id:
                        tptr = nameref(&pnode);
                        break;
                case iconst:
                        tptr = &stdint;
                        pnode = makeinode(en_icon,ival);
                        pnode->constflag = TRUE;
                        NextToken();
                        break;
                case rconst:
                        tptr = &stddbl;
                        pnode = makefnode(en_fcon,rval);
                        pnode->constflag = TRUE;
                        NextToken();
                        break;
                case sconst:
					if (sizeof_flag) {
						tptr = maketype(bt_pointer, 0);
						tptr->size = strlen(laststr) + 1;
						tptr->btp = &stdchar;
						tptr->val_flag = 1;
						tptr->isConst = TRUE;
					}
					else {
                        tptr = &stdstring;
					}
                    pnode = makenodei(en_labcon,NULL,NULL);
					if (sizeof_flag == 0)
						pnode->i = stringlit(laststr);
					pnode->etype = bt_pointer;
					pnode->esize = 8;
                    pnode->constflag = TRUE;
                    NextToken();
                    break;

                case openpa:
                        NextToken();
                        if( !IsBeginningOfTypecast(lastst) ) {
                            tptr = expression(&pnode);
                            needpunc(closepa);
                        }
                        else {			/* cast operator */
                            ParseSpecifier(0); /* do cast ParseSpecifieraration */
                            ParseDeclarationPrefix(FALSE);
                            tptr = head;
                            needpunc(closepa);
                            if( ParseUnaryExpression(&pnode) == NULL ) {
                                error(ERR_IDEXPECT);
                                tptr = NULL;
                            }
                        }
                        break;

                default:
                        return NULL;
                }
        for(;;) {
                switch( lastst ) {
                        case openbr:    /* build a subscript reference */
							brcount++;
							if (tptr==NULL) {
								error(ERR_UNDEFINED);
								goto fini;
							}
                                if( tptr->type != bt_pointer )
                                        error(ERR_NOPOINTER);
                                else
                                        tptr = tptr->btp;
                                NextToken();
								if (tptr->val_flag && (tptr->size==1 || tptr->size==2 || tptr->size==4 || tptr->size==8)) {
									expression(&rnode);
									pnode = makenode(en_add,rnode,pnode);
									pnode->constflag = rnode->constflag && pnode->p[1]->constflag;
									pnode->isUnsigned = rnode->isUnsigned && pnode->p[1]->isUnsigned;
									pnode->scale = tptr->size;
//									needpunc(closebr);
								}
								else {
									sza[brcount-1] = tptr->size;
									qnode = makeinode(en_icon,tptr->size);
									// swap sizes for array indexing
									if (brcount==3) {
										qnode->i = sza[0];
										qnode2->i = sza[1];
										qnode1->i = sza[2];
									}
									else if (brcount==2) {
										qnode->i = sza[0];
										qnode1->i = sza[1];
									}
									if (qnode1==NULL)
										qnode1 = qnode;
									else
										qnode2 = qnode;
									qnode->constflag = TRUE;
									qnode->isUnsigned = tptr->isUnsigned;
									expression(&rnode);
//							needpunc(closebr);
	/*
 *      we could check the type of the expression here...
									
 */
									if (rnode==NULL) {
										error(ERR_EXPREXPECT);
										break;
									}
									qnode = makenode(en_mulu,rnode,qnode);
									qnode->constflag = rnode->constflag && qnode->p[0]->constflag;
									pnode = makenode(en_add,qnode,pnode);
									pnode->constflag = qnode->constflag && pnode->p[1]->constflag;
									pnode->scale = 1;
									//if( tptr->val_flag == 0 )
									//	tptr = deref(&pnode,tptr);
								}
                                ////snode = makenode(en_mul,qnode,rnode);
                                ////snode->constflag = rnode->constflag && snode->p[0]->constflag;
                                ////pnode = makenode(en_add,snode,pnode);
                                ////pnode->constflag = snode->constflag && pnode->p[1]->constflag;
                                tptr = CondDeref(&pnode,tptr);
                                needpunc(closebr);
                                break;

                        case pointsto:
							if (tptr==NULL) {
								error(ERR_UNDEFINED);
								goto fini;
							}
                            if( tptr->type != bt_pointer )
                                error(ERR_NOPOINTER);
                            else
                                tptr = tptr->btp;
                            if( tptr->val_flag == FALSE )
                                pnode = makenode(en_w_ref,pnode,NULL);
/*
 *      fall through to dot operation
 */
                        case dot:
							if (tptr==NULL) {
								error(ERR_UNDEFINED);
								goto fini;
							}
                                NextToken();       /* past -> or . */
                                if( lastst != id )
                                        error(ERR_IDEXPECT);
                                else    {
                                        sp = search(lastid,&tptr->lst);
                                        if( sp == NULL )
                                            error(ERR_NOMEMBER);
                                        else {
                                            tptr = sp->tp;
                                            qnode = makeinode(en_icon,sp->value.i);
                                            qnode->constflag = TRUE;
                                            pnode = makenode(en_add,pnode,qnode);
                                            pnode->constflag = pnode->p[0]->constflag;
                                            tptr = CondDeref(&pnode,tptr);
                                        }
                                        NextToken();       /* past id */
                                        }
                                break;

                        case openpa:    /* function reference */
                                NextToken();
                                if( tptr->type != bt_func && tptr->type != bt_ifunc )
                                    error(ERR_NOFUNC);
                                else
                                    tptr = tptr->btp;
								if (currentFn==NULL)
									error(ERR_SYNTAX);
								else
									currentFn->IsLeaf = FALSE;
                                pnode = makenode(en_fcall,pnode,ArgumentList());
                                needpunc(closepa);
                                break;

                        default:
							//if (tptr->type == bt_func || tptr->type== bt_ifunc) {
       //                         tptr = deref(&pnode,tptr);
							//}
                            goto fini;
                        }
                }
fini:   *node = pnode;
        return tptr;
}

/*
 *      this function returns true if the node passed is an IsLValue.
 *      this can be qualified by the fact that an IsLValue must have
 *      one of the dereference operators as it's top node.
 */
int IsLValue(ENODE *node)
{
	switch( node->nodetype ) {
    case en_b_ref:
	case en_c_ref:
	case en_h_ref:
    case en_w_ref:
	case en_ub_ref:
	case en_uc_ref:
	case en_uh_ref:
    case en_uw_ref:
	case en_wfieldref:
	case en_uwfieldref:
	case en_bfieldref:
	case en_ubfieldref:
	case en_cfieldref:
	case en_ucfieldref:
	case en_hfieldref:
	case en_uhfieldref:
	case en_dbl_ref:
	case en_flt_ref:
	//case en_struct_ref:
            return TRUE;
	case en_cbc:
	case en_cbh:
    case en_cbw:
	case en_cch:
	case en_ccw:
	case en_chw:
	case en_cfd:
            return IsLValue(node->p[0]);
    }
    return FALSE;
}

/*
 *      ParseUnaryExpression evaluates unary expressions and returns the type of the
 *      expression evaluated. unary expressions are any of:
 *
 *                      primary
 *                      primary++
 *                      primary--
 *                      !unary
 *                      ~unary
 *                      ++unary
 *                      --unary
 *                      -unary
 *                      *unary
 *                      &unary
 *                      (typecast)unary
 *                      sizeof(typecast)
 *                      typenum(typecast)
 *
 */
TYP *ParseUnaryExpression(ENODE **node)
{
	TYP *tp, *tp1;
    ENODE *ep1, *ep2;
    int flag, flag2;
	__int64 i;
	SYM *sp;

        flag = 0;
		flag2 = FALSE;
        switch( lastst ) {
                case autodec:
                        flag = 1;
                /* fall through to common increment */
                case autoinc:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return NULL;
                        }
                        if( IsLValue(ep1)) {
                            if( tp->type == bt_pointer )
                                ep2 = makeinode(en_icon,tp->btp->size);
                            else
                                ep2 = makeinode(en_icon,1);
                            ep2->constflag = TRUE;
                            ep1 = makenode(flag ? en_assub : en_asadd,ep1,ep2);
                        }
                        else
                            error(ERR_LVALUE);
                        break;

                case minus:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return NULL;
                        }
                        ep1 = makenode(en_uminus,ep1,NULL);
                        ep1->constflag = ep1->p[0]->constflag;
						ep1->isUnsigned = ep1->p[0]->isUnsigned;
                        break;

                case not:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return NULL;
                        }
                        ep1 = makenode(en_not,ep1,NULL);
                        ep1->constflag = ep1->p[0]->constflag;
                        break;

                case compl:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return 0;
                        }
                        ep1 = makenode(en_compl,ep1,NULL);
                        ep1->constflag = ep1->p[0]->constflag;
                        break;

                case star:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return NULL;
                        }
                        if( tp->btp == NULL )
							error(ERR_DEREF);
                        else
                            tp = tp->btp;
						tp = CondDeref(&ep1,tp);
                        break;

                case and:
                        NextToken();
                        tp = ParseUnaryExpression(&ep1);
                        if( tp == NULL ) {
                            error(ERR_IDEXPECT);
                            return NULL;
                        }
                        if( IsLValue(ep1))
                                ep1 = ep1->p[0];
                        tp1 = allocTYP();
                        tp1->size = 8;
                        tp1->type = bt_pointer;
                        tp1->btp = tp;
                        tp1->val_flag = FALSE;
                        tp1->lst.head = NULL;
                        tp1->sname = NULL;
                        tp = tp1;
						sp = search("ta_int",&tp->btp->lst);
						if (sp) {
							printf("ta_int\r\n");
						}
                        break;

                case kw_sizeof:
                        NextToken();
						if (lastst==openpa) {
							flag2 = TRUE;
							NextToken();
						}
						if (flag2 && IsBeginningOfTypecast(lastst)) {
							tp = head;
							tp1 = tail;
							ParseSpecifier(0);
							ParseDeclarationPrefix(FALSE);
							if( head != NULL )
								ep1 = makeinode(en_icon,head->size);
							else {
								error(ERR_IDEXPECT);
								ep1 = makeinode(en_icon,1);
							}
							head = tp;
							tail = tp1;
						}
						else {
							sizeof_flag++;
							tp = ParseUnaryExpression(&ep1);
							sizeof_flag--;
							if (tp == 0) {
								error(ERR_SYNTAX);
								ep1 = makeinode(en_icon,1);
							} else
								ep1 = makeinode(en_icon, (long) tp->size);
						}
						if (flag2)
							needpunc(closepa);
						ep1->constflag = TRUE;
						tp = &stdint;
                        break;

                case kw_typenum:
                        NextToken();
                        needpunc(openpa);
						tp = head;
						tp1 = tail;
                        ParseSpecifier(0);
                        ParseDeclarationPrefix(FALSE);
                        if( head != NULL )
                            ep1 = makeinode(en_icon,GetTypeHash(head));
						else {
                            error(ERR_IDEXPECT);
                            ep1 = makeinode(en_icon,1);
                        }
						head = tp;
						tail = tp1;
                        ep1->constflag = TRUE;
                        tp = &stdint;
                        needpunc(closepa);
                        break;

                default:
                        tp = ParsePrimaryExpression(&ep1);
                        if( tp != NULL ) {
                                if( tp->type == bt_pointer )
                                        i = tp->btp->size;
                                else
                                        i = 1;
                                if( lastst == autoinc) {
									if( IsLValue(ep1) ) {
										if (tp->type == bt_pointer)
											ep2 = makeinode(en_icon,tp->btp->size);
										else
											ep2 = makeinode(en_icon,1);
										ep2->constflag = TRUE;
										ep1 = makenode(en_asadd,ep1,ep2);
//                                        ep1 = makenodei(en_ainc,ep1,i);
									}
                                    else
                                        error(ERR_LVALUE);
                                    NextToken();
                                }
                                else if( lastst == autodec ) {
									if( IsLValue(ep1) ) {
										if (tp->type == bt_pointer)
											ep2 = makeinode(en_icon,tp->btp->size);
										else
											ep2 = makeinode(en_icon,1);
										ep2->constflag = TRUE;
										ep1 = makenode(en_assub,ep1,ep2);
//                                                ep1 = makenodei(en_adec,ep1,i);
										}
                                        else
                                                error(ERR_LVALUE);
                                        NextToken();
                                        }
                                }
                        break;
                }
        *node = ep1;
        return tp;
}

/*
 *      forcefit will coerce the nodes passed into compatable
 *      types and return the type of the resulting expression.
 */
TYP     *forcefit(ENODE **node1,TYP *tp1,ENODE **node2,TYP *tp2)
{
	switch( tp1->type ) {
				case bt_byte:
					if (tp2->type == bt_long) {
						if (tp1->isUnsigned)
							return &stdulong;
						else
							return &stdlong;
					}
					if (tp2->type == bt_short) {
						if (tp1->isUnsigned)
							return &stdushort;
						else
							return &stdshort;
					}
					if (tp2->type == bt_char) {
						if (tp1->isUnsigned)
							return &stduchar;
						else
							return &stdchar;
					}
					if (tp2->type == bt_pointer)
						return tp2;
					//if (tp2->type == bt_double || tp2->type == bt_float) {
					//	*node1 = makenode(en_i2d,*node1,*node2);
					//	return tp2;
					//}
					return tp1;	// char
                case bt_char:
                case bt_uchar:
					if (tp2->type == bt_long) {
						if (tp1->isUnsigned)
							return &stdulong;
						else
							return &stdlong;
					}
					if (tp2->type == bt_short) {
						if (tp1->isUnsigned)
							return &stdushort;
						else
							return &stdshort;
					}
					if (tp2->type == bt_pointer)
						return tp2;
					//if (tp2->type == bt_double || tp2->type == bt_float) {
					//	*node1 = makenode(en_i2d,*node1,*node2);
					//	return tp2;
					//}
					return tp1;	// char
                case bt_short:
                case bt_ushort:
					if (tp2->type == bt_long) {
						if (tp1->isUnsigned)
							return &stdulong;
						else
							return &stdlong;
					}
					if (tp2->type == bt_pointer)
						return tp2;
					//if (tp2->type == bt_double || tp2->type == bt_float) {
					//	*node1 = makenode(en_i2d,*node1,*node2);
					//	return tp2;
					//}
					return tp1;
                case bt_long:
                case bt_ulong:
                        if( tp2->type == bt_pointer	)
							return tp2;
						//if (tp2->type == bt_double || tp2->type == bt_float) {
						//	*node1 = makenode(en_i2d,*node1,*node2);
						//	return tp2;
						//}
						return tp1;
                case bt_pointer:
                        if( isscalar(tp2) || tp2->type == bt_pointer)
                                return tp1;
						// pointer to function was really desired.
						if (tp2->type == bt_func || tp2->type==bt_ifunc)
							return tp1;
                        break;
                case bt_unsigned:
                        if( tp2->type == bt_pointer )
                                return tp2;
                        else if( isscalar(tp2) )
                                return tp1;
                        break;
				case bt_float:
						if (tp2->type == bt_double)
							return &stddbl;
						return tp1;
				case bt_double:
						if (tp2->type == bt_long)
							*node2 = makenode(en_i2d,*node2,*node1);
						return tp1;
				case bt_struct:
				case bt_union:
						if (tp2->size > tp1->size)
							return tp2;
						return tp1;
				// Really working with pointers to functions.
				case bt_func:
				case bt_ifunc:
						return tp1;
                }
        error( ERR_MISMATCH );
        return tp1;
}

/*
 *      this function returns true when the type of the argument is
 *      one of char, short, unsigned, or long.
 */
int     isscalar(TYP *tp)
{
	return
			tp->type == bt_byte ||
			tp->type == bt_char ||
            tp->type == bt_short ||
            tp->type == bt_long ||
			tp->type == bt_uchar ||
            tp->type == bt_ushort ||
            tp->type == bt_ulong ||
            tp->type == bt_unsigned;
}

/*
 *      multops parses the multiply priority operators. the syntax of
 *      this group is:
 *
 *              unary
 *              multop * unary
 *              multop / unary
 *              multop % unary
 */
TYP *multops(struct ENODE **node)
{
	struct ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        int		      oper;
        tp1 = ParseUnaryExpression(&ep1);
        if( tp1 == 0 )
                return 0;
        while( lastst == star || lastst == divide || lastst == modop ) {
                oper = lastst;
                NextToken();       /* move on to next unary op */
                tp2 = ParseUnaryExpression(&ep2);
                if( tp2 == 0 ) {
                        error(ERR_IDEXPECT);
                        *node = ep1;
                        return tp1;
                        }
                tp1 = forcefit(&ep1,tp1,&ep2,tp2);
                switch( oper ) {
                        case star:
								if (tp1->type==bt_double)
									ep1 = makenode(en_fdmul,ep1,ep2);
								else if (tp1->type==bt_float)
									ep1 = makenode(en_fsmul,ep1,ep2);
								else if( tp1->isUnsigned )
                                        ep1 = makenode(en_mulu,ep1,ep2);
                                else
                                        ep1 = makenode(en_mul,ep1,ep2);
                                break;
                        case divide:
								if (tp1->type==bt_double)
									ep1 = makenode(en_fddiv,ep1,ep2);
								else if (tp1->type==bt_float)
									ep1 = makenode(en_fsdiv,ep1,ep2);
                                else if( tp1->isUnsigned )
                                    ep1 = makenode(en_udiv,ep1,ep2);
                                else
                                    ep1 = makenode(en_div,ep1,ep2);
                                break;
                        case modop:
                                if( tp1->isUnsigned )
                                        ep1 = makenode(en_umod,ep1,ep2);
                                else
                                        ep1 = makenode(en_mod,ep1,ep2);
                                break;
                        }
                PromoteConstFlag(ep1);
                }
        *node = ep1;
        return tp1;
}

/*
 *      addops handles the addition and subtraction operators.
 */
TYP     *addops(ENODE **node)
{
	ENODE    *ep1, *ep2, *ep3;
    TYP             *tp1, *tp2;
    int             oper;

	tp1 = multops(&ep1);
    if( tp1 == NULL )
        return NULL;
    while( lastst == plus || lastst == minus ) {
            oper = (lastst == plus);
            NextToken();
            tp2 = multops(&ep2);
            if( tp2 == 0 ) {
                    error(ERR_IDEXPECT);
                    *node = ep1;
                    return tp1;
                    }
            if( tp1->type == bt_pointer ) {
                    tp2 = forcefit(0,&stdint,&ep2,tp2);
                    ep3 = makeinode(en_icon,tp1->btp->size);
                    ep3->constflag = TRUE;
                    ep2 = makenode(en_mulu,ep3,ep2);
                    ep2->constflag = ep2->p[1]->constflag;
                    }
            else if( tp2->type == bt_pointer ) {
                    tp1 = forcefit(0,&stdint,&ep1,tp1);
                    ep3 = makeinode(en_icon,tp2->btp->size);
                    ep3->constflag = TRUE;
                    ep1 = makenode(en_mulu,ep3,ep1);
                    ep1->constflag = ep1->p[1]->constflag;
                    }
            tp1 = forcefit(&ep1,tp1,&ep2,tp2);
			if (tp1->type==bt_double)
				ep1 = makenode( oper ? en_fdadd : en_fdsub,ep1,ep2);
			else if (tp1->type==bt_float)
				ep1 = makenode( oper ? en_fsadd : en_fssub,ep1,ep2);
			else
				ep1 = makenode( oper ? en_add : en_sub,ep1,ep2);
            PromoteConstFlag(ep1);
            }
    *node = ep1;
    return tp1;
}

/*
 *      shiftop handles the shift operators << and >>.
 */
TYP     *shiftop(ENODE **node)
{
	struct ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        int             oper;
        tp1 = addops(&ep1);
        if( tp1 == 0)
                return 0;
        while( lastst == lshift || lastst == rshift) {
                oper = (lastst == lshift);
                NextToken();
                tp2 = addops(&ep2);
                if( tp2 == 0 )
                        error(ERR_IDEXPECT);
                else    {
                        tp1 = forcefit(&ep1,tp1,&ep2,tp2);
						if (tp1->type==bt_float||tp1->type==bt_double)
							error(ERR_UNDEF_OP);
						else {
							if (tp1->isUnsigned)
								ep1 = makenode(oper ? en_shlu : en_shru,ep1,ep2);
							else
								ep1 = makenode(oper ? en_shl : en_asr,ep1,ep2);
							PromoteConstFlag(ep1);
							}
                        }
                }
        *node = ep1;
        return tp1;
}

/*
 *      relation handles the relational operators < <= > and >=.
 */
TYP     *relation(ENODE **node)
{       struct ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        int             nt;
        tp1 = shiftop(&ep1);
        if( tp1 == 0 )
                return 0;
        for(;;) {
                switch( lastst ) {

                        case lt:
                                if( tp1->isUnsigned )
                                        nt = en_ult;
                                else
                                        nt = en_lt;
                                break;
                        case gt:
                                if( tp1->isUnsigned )
                                        nt = en_ugt;
                                else
                                        nt = en_gt;
                                break;
                        case leq:
                                if( tp1->isUnsigned )
                                        nt = en_ule;
                                else
                                        nt = en_le;
                                break;
                        case geq:
                                if( tp1->isUnsigned )
                                        nt = en_uge;
                                else
                                        nt = en_ge;
                                break;
                        default:
                                goto fini;
                        }
                NextToken();
                tp2 = shiftop(&ep2);
                if( tp2 == 0 )
                        error(ERR_IDEXPECT);
                else    {
                        tp1 = forcefit(&ep1,tp1,&ep2,tp2);
                        ep1 = makenode(nt,ep1,ep2);
		                PromoteConstFlag(ep1);
                        }
                }
fini:   *node = ep1;
        return tp1;
}

/*
 *      equalops handles the equality and inequality operators.
 */
TYP     *equalops(ENODE **node)
{
	ENODE    *ep1, *ep2;
    TYP             *tp1, *tp2;
    int             oper;
    tp1 = relation(&ep1);
    if( tp1 == NULL )
            return NULL;
    while( lastst == eq || lastst == neq ) {
        oper = (lastst == eq);
        NextToken();
        tp2 = relation(&ep2);
        if( tp2 == NULL )
                error(ERR_IDEXPECT);
        else {
            tp1 = forcefit(&ep1,tp1,&ep2,tp2);
            ep1 = makenode( oper ? en_eq : en_ne,ep1,ep2);
            PromoteConstFlag(ep1);
        }
	}
    *node = ep1;
    return tp1;
}

/*
 *      binop is a common routine to handle all of the legwork and
 *      error checking for bitand, bitor, bitxor, andop, and orop.
 */
TYP *binop(ENODE **node, TYP *(*xfunc)(),int nt, int sy)
{
	ENODE    *ep1, *ep2;
        TYP             *tp1, *tp2;
        tp1 = (*xfunc)(&ep1);
        if( tp1 == 0 )
                return 0;
        while( lastst == sy ) {
                NextToken();
                tp2 = (*xfunc)(&ep2);
                if( tp2 == 0 )
                        error(ERR_IDEXPECT);
                else    {
                        tp1 = forcefit(&ep1,tp1,&ep2,tp2);
                        ep1 = makenode(nt,ep1,ep2);
		                PromoteConstFlag(ep1);
                        }
                }
        *node = ep1;
        return tp1;
}

TYP     *bitand(ENODE **node)
/*
 *      the bitwise and operator...
 */
{       return binop(node,equalops,en_and,and);
}

TYP     *bitxor(ENODE **node)
{       return binop(node,bitand,en_xor,uparrow);
}

TYP     *bitor(ENODE **node)
{       return binop(node,bitxor,en_or,or);
}

TYP     *andop(ENODE **node)
{       return binop(node,bitor,en_land,land);
}

TYP *orop(ENODE **node)
{
	return binop(node,andop,en_lor,lor);
}

/*
 *      this routine processes the hook operator.
 */
TYP *conditional(ENODE **node)
{
	TYP             *tp1, *tp2, *tp3;
    struct ENODE    *ep1, *ep2, *ep3;
    tp1 = orop(&ep1);       /* get condition */
    if( tp1 == NULL )
            return NULL;
    if( lastst == hook ) {
			iflevel++;
            NextToken();
            if( (tp2 = conditional(&ep2)) == NULL) {
                    error(ERR_IDEXPECT);
                    goto cexit;
                    }
            needpunc(colon);
            if( (tp3 = conditional(&ep3)) == NULL) {
                    error(ERR_IDEXPECT);
                    goto cexit;
                    }
            tp1 = forcefit(&ep2,tp2,&ep3,tp3);
            ep2 = makenode(en_void,ep2,ep3);
            ep1 = makenode(en_cond,ep1,ep2);
			iflevel--;
            }
cexit:  *node = ep1;
    return tp1;
}

// ----------------------------------------------------------------------------
//      asnop handles the assignment operators. currently only the
//      simple assignment is implemented.
// ----------------------------------------------------------------------------
TYP *asnop(ENODE **node)
{      
	struct ENODE    *ep1, *ep2, *ep3;
    TYP             *tp1, *tp2;
    int             op;
    tp1 = conditional(&ep1);
    if( tp1 == 0 )
        return 0;
    for(;;) {
        switch( lastst ) {
            case assign:
                op = en_assign;
ascomm:         NextToken();
                tp2 = asnop(&ep2);
ascomm2:        if( tp2 == 0 || !IsLValue(ep1) )
                    error(ERR_LVALUE);
				else {
					tp1 = forcefit(&ep1,tp1,&ep2,tp2);
					ep1 = makenode(op,ep1,ep2);
					// Struct assign calls memcpy, so function is no
					// longer a leaf routine.
					if (tp1->size > 8)
						currentFn->IsLeaf = FALSE;
				}
				break;
			case asplus:
				op = en_asadd;
ascomm3:        tp2 = asnop(&ep2);
				if( tp1->type == bt_pointer ) {
					ep3 = makeinode(en_icon,tp1->btp->size);
					ep2 = makenode(en_mul,ep2,ep3);
				}
				goto ascomm;
			case asminus:
				op = en_assub;
				goto ascomm3;
			case astimes:
				if (tp1->isUnsigned)
					op = en_asmulu;
				else
					op = en_asmul;
				goto ascomm;
			case asdivide:
				if (tp1->isUnsigned)
					op = en_asdivu;
				else
					op = en_asdiv;
				goto ascomm;
			case asmodop:
				if (tp1->isUnsigned)
					op = en_asmodu;
				else
					op = en_asmod;
				goto ascomm;
			case aslshift:
				op = en_aslsh;
				goto ascomm;
			case asrshift:
				if (tp1->isUnsigned)
					op = en_asrshu;
				else
					op = en_asrsh;
				goto ascomm;
			case asand:
				op = en_asand;
				goto ascomm;
			case asor:
				op = en_asor;
				goto ascomm;
			default:
				goto asexit;
			}
	}
asexit: *node = ep1;
        return tp1;
}

/*
 *      evaluate an expression where the comma operator is not legal.
 */
TYP *NonCommaExpression(ENODE **node)
{
	TYP *tp;
    tp = asnop(node);
    if( tp == NULL )
        *node = NULL;
    return tp;
}

/*
 *      evaluate the comma operator. comma operators are kept as
 *      void nodes.
 */
TYP *commaop(ENODE **node)
{
	TYP *tp1,*tp2;
	ENODE *ep1,*ep2;

	tp1 = NonCommaExpression(&ep1);
	if (tp1==NULL)
		return NULL;
	while(1) {
		if (lastst==comma) {
			NextToken();
			tp2 = NonCommaExpression(&ep2);
            ep1 = makenode(en_void,ep1,ep2);
		}
		else
			break;
	}
	*node = ep1;
	return tp1;
}

//TYP *commaop(ENODE **node)
//{  
//	TYP             *tp1;
//        ENODE    *ep1, *ep2;
//        tp1 = asnop(&ep1);
//        if( tp1 == NULL )
//                return NULL;
//        if( lastst == comma ) {
//				NextToken();
//                tp1 = commaop(&ep2);
//                if( tp1 == NULL ) {
//                        error(ERR_IDEXPECT);
//                        goto coexit;
//                        }
//                ep1 = makenode(en_void,ep1,ep2);
//                }
//coexit: *node = ep1;
//        return tp1;
//}

/*
 *      evaluate an expression where all operators are legal.
 */
TYP *expression(ENODE **node)
{
	TYP *tp;
    tp = commaop(node);
    if( tp == NULL )
        *node = NULL;
    return tp;
}
