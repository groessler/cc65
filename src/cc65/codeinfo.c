/*****************************************************************************/
/*                                                                           */
/*				  codeinfo.c				     */
/*                                                                           */
/*		    Additional information about 6502 code		     */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2001      Ullrich von Bassewitz                                       */
/*               Wacholderweg 14                                             */
/*               D-70597 Stuttgart                                           */
/* EMail:        uz@cc65.org                                                 */
/*                                                                           */
/*                                                                           */
/* This software is provided 'as-is', without any expressed or implied       */
/* warranty.  In no event will the authors be held liable for any damages    */
/* arising from the use of this software.                                    */
/*                                                                           */
/* Permission is granted to anyone to use this software for any purpose,     */
/* including commercial applications, and to alter it and redistribute it    */
/* freely, subject to the following restrictions:                            */
/*                                                                           */
/* 1. The origin of this software must not be misrepresented; you must not   */
/*    claim that you wrote the original software. If you use this software   */
/*    in a product, an acknowledgment in the product documentation would be  */
/*    appreciated but is not required.                                       */
/* 2. Altered source versions must be plainly marked as such, and must not   */
/*    be misrepresented as being the original software.                      */
/* 3. This notice may not be removed or altered from any source              */
/*    distribution.                                                          */
/*                                                                           */
/*****************************************************************************/



#include <stdlib.h>
#include <string.h>

/* common */
#include "coll.h"

/* cc65 */
#include "codeent.h"
#include "codeseg.h"
#include "datatype.h"
#include "error.h"
#include "symtab.h"
#include "codeinfo.h"



/*****************************************************************************/
/*     	       	      	  	     Data				     */
/*****************************************************************************/



/* Table listing the function names and code info values for known internally
 * used functions. This table should get auto-generated in the future.
 */
typedef struct FuncInfo FuncInfo;
struct FuncInfo {
    const char*	    Name;	/* Function name */
    unsigned short  Use;	/* Register usage */
    unsigned short  Chg;   	/* Changed/destroyed registers */
};

static const FuncInfo FuncInfoTable[] = {
    { "addysp",	       	REG_Y, 	       	REG_NONE	                   },
    { "aslax1",         REG_AX,	       	REG_AX | REG_TMP1		   },
    { "aslax2",         REG_AX,		REG_AX | REG_TMP1		   },
    { "aslax3",         REG_AX,		REG_AX | REG_TMP1		   },
    { "aslax4",         REG_AX,		REG_AX | REG_TMP1		   },
    { "bnega",          REG_A,          REG_AX				   },
    { "bnegax",         REG_AX,         REG_AX				   },
    { "bnegeax",        REG_EAX,        REG_EAX				   },
    { "booleq",		REG_NONE,    	REG_AX				   },
    { "boolge",		REG_NONE,	REG_AX				   },
    { "boolgt",		REG_NONE,	REG_AX				   },
    { "boolle",		REG_NONE,	REG_AX				   },
    { "boollt",		REG_NONE,	REG_AX				   },
    { "boolne",		REG_NONE,	REG_AX				   },
    { "booluge",   	REG_NONE,	REG_AX				   },
    { "boolugt",   	REG_NONE,	REG_AX				   },
    { "boolule",   	REG_NONE,	REG_AX				   },
    { "boolult",   	REG_NONE,	REG_AX				   },
    { "complax",        REG_AX,         REG_AX				   },
    { "decax1",     	REG_AX,		REG_AX				   },
    { "decax2",     	REG_AX,		REG_AX				   },
    { "decax3",     	REG_AX,		REG_AX				   },
    { "decax4",     	REG_AX,		REG_AX				   },
    { "decax5",     	REG_AX,		REG_AX				   },
    { "decax6",     	REG_AX,		REG_AX				   },
    { "decax7",     	REG_AX,		REG_AX				   },
    { "decax8",     	REG_AX,		REG_AX				   },
    { "decaxy",	   	REG_AXY,	REG_AX | REG_TMP1		   },
    { "decsp1",        	REG_NONE,      	REG_Y				   },
    { "decsp2",    	REG_NONE,	REG_A				   },
    { "decsp3",    	REG_NONE,	REG_A				   },
    { "decsp4",    	REG_NONE,	REG_A				   },
    { "decsp5",    	REG_NONE,	REG_A				   },
    { "decsp6",     	REG_NONE,	REG_A				   },
    { "decsp7",    	REG_NONE,	REG_A				   },
    { "decsp8",      	REG_NONE,	REG_A				   },
    { "incax1",         REG_AX,         REG_AX				   },
    { "incax2",         REG_AX,         REG_AX				   },
    { "incsp1",		REG_NONE,	REG_NONE			   },
    { "incsp2",	    	REG_NONE,	REG_Y				   },
    { "incsp3",		REG_NONE,	REG_Y				   },
    { "incsp4",		REG_NONE,	REG_Y				   },
    { "incsp5",		REG_NONE,	REG_Y				   },
    { "incsp6",		REG_NONE,	REG_Y				   },
    { "incsp7",		REG_NONE,	REG_Y				   },
    { "incsp8",	    	REG_NONE,	REG_Y				   },
    { "ldaidx",         REG_AXY,        REG_AX | REG_PTR1		   },
    { "ldauidx",        REG_AXY,        REG_AX | REG_PTR1		   },
    { "ldax0sp",   	REG_Y,		REG_AX				   },
    { "ldaxi",          REG_AX,         REG_AXY | REG_PTR1		   },
    { "ldaxidx",        REG_AXY,        REG_AX | REG_PTR1		   },
    { "ldaxysp",   	REG_Y,		REG_AX				   },
    { "leaasp",         REG_A,          REG_AX				   },
    { "negax",          REG_AX,         REG_AX				   },
    { "pusha",	   	REG_A,		REG_Y				   },
    { "pusha0",	       	REG_A,		REG_XY				   },
    { "pushax",	   	REG_AX,		REG_Y				   },
    { "pusheax",        REG_EAX,        REG_Y				   },
    { "pushw0sp",  	REG_NONE,	REG_AXY				   },
    { "pushwysp",  	REG_Y,		REG_AXY				   },
    { "shlax1",         REG_AX,		REG_AX | REG_TMP1		   },
    { "shlax2",         REG_AX,		REG_AX | REG_TMP1		   },
    { "shlax3",         REG_AX,		REG_AX | REG_TMP1		   },
    { "shlax4",         REG_AX,		REG_AX | REG_TMP1		   },
    { "shrax1",         REG_AX,         REG_AX | REG_TMP1		   },
    { "shrax2",         REG_AX,         REG_AX | REG_TMP1		   },
    { "shrax3",         REG_AX,         REG_AX | REG_TMP1		   },
    { "shrax4",         REG_AX,         REG_AX | REG_TMP1		   },
    { "shreax1",        REG_EAX,        REG_AX | REG_TMP1		   },
    { "shreax2",        REG_EAX,        REG_AX | REG_TMP1		   },
    { "shreax3",        REG_EAX,        REG_AX | REG_TMP1		   },
    { "shreax4",        REG_EAX,        REG_AX | REG_TMP1		   },
    { "staspidx",       REG_A | REG_Y,  REG_Y | REG_TMP1 | REG_PTR1	   },
    { "stax0sp",        REG_AX,         REG_Y				   },
    { "tosicmp",       	REG_AX,	       	REG_AXY | REG_SREG		   },
    { "tosdiva0",       REG_AX,	       	REG_ALL				   },
    { "tosdivax",       REG_AX,	       	REG_ALL				   },
    { "tosdiveax",      REG_EAX,       	REG_ALL				   },
    { "tosmula0",       REG_AX,	       	REG_ALL				   },
    { "tosmulax",       REG_AX,	       	REG_ALL				   },
    { "tosmuleax",      REG_EAX,       	REG_ALL				   },
    { "tosshreax",      REG_EAX,       	REG_EAXY | REG_PTR1 | REG_PTR2	   },
    { "tosumula0",      REG_AX,	       	REG_ALL				   },
    { "tosumulax",      REG_AX,	       	REG_ALL				   },
    { "tosumuleax",     REG_EAX,       	REG_ALL				   },
};
#define FuncInfoCount	(sizeof(FuncInfoTable) / sizeof(FuncInfoTable[0]))

/* Table with names of zero page locations used by the compiler */
typedef struct ZPInfo ZPInfo;
struct ZPInfo {
    unsigned char  Len;		/* Length of the following string */
    char           Name[11];    /* Name of zero page symbol */
    unsigned short RegInfo;     /* Register info for this symbol */
};
static const ZPInfo ZPInfoTable[] = {
    {  	4,     	"ptr1",         REG_PTR1        },
    {  	4,     	"ptr2",         REG_PTR2        },
    {  	4,     	"ptr3",         REG_PTR3        },
    {  	4,     	"ptr4",         REG_PTR4        },
    {   7,      "regbank",      REG_NONE        },
    {   7,      "regsave",      REG_NONE        },
    {   2,      "sp",           REG_NONE        },
    {   4,      "sreg",         REG_SREG        },
    {   4,      "tmp1",         REG_TMP1        },
    {   4,      "tmp2",         REG_TMP2        },
    {   4,      "tmp3",         REG_TMP3        },
};
#define ZPInfoCount    	(sizeof(ZPInfoTable) / sizeof(ZPInfoTable[0]))


/*****************************************************************************/
/*     	       	      	  	     Code 	    	 	       	     */
/*****************************************************************************/



static int CompareFuncInfo (const void* Key, const void* Info)
/* Compare function for bsearch */
{
    return strcmp (Key, ((const	FuncInfo*) Info)->Name);
}



void GetFuncInfo (const char* Name, unsigned short* Use, unsigned short* Chg)
/* For the given function, lookup register information and store it into
 * the given variables. If the function is unknown, assume it will use and
 * load all registers.
 */
{
    /* If the function name starts with an underline, it is an external
     * function. Search for it in the symbol table. If the function does
     * not start with an underline, it may be a runtime support function.
     * Search for it in the list of builtin functions.
     */
    if (Name[0] == '_') {

     	/* Search in the symbol table, skip the leading underscore */
     	SymEntry* E = FindGlobalSym (Name+1);

     	/* Did we find it in the top level table? */
     	if (E && IsTypeFunc (E->Type)) {

     	    /* A function may use the A or A/X registers if it is a fastcall
     	     * function. If it is not a fastcall function but a variadic one,
	     * it will use the Y register (the parameter size is passed here).
	     * In all other cases, no registers are used. However, we assume
	     * that any function will destroy all registers.
     	     */
     	    FuncDesc* D = E->V.F.Func;
     	    if ((D->Flags & FD_FASTCALL) != 0 && D->ParamCount > 0) {
     		/* Will use registers depending on the last param */
     		SymEntry* LastParam = D->SymTab->SymTail;
     		if (SizeOf (LastParam->Type) == 1) {
     		    *Use = REG_A;
     		} else {
     		    *Use = REG_AX;
     		}
	    } else if ((D->Flags & FD_VARIADIC) != 0) {
		*Use = REG_Y;
     	    } else {
     		/* Will not use any registers */
     		*Use = REG_NONE;
     	    }

     	    /* Will destroy all registers */
     	    *Chg = REG_ALL;

     	    /* Done */
     	    return;
     	}

    } else {

	/* Search for the function in the list of builtin functions */
	const FuncInfo* Info = bsearch (Name, FuncInfoTable, FuncInfoCount,
					sizeof(FuncInfo), CompareFuncInfo);

	/* Do we know the function? */
	if (Info) {
	    /* Use the information we have */
	    *Use = Info->Use;
	    *Chg = Info->Chg;
	    return;
	}
    }

    /* Function not found - assume all CPU registers are input, and all
     * registers are changed
     */
    *Use = REG_AXY;
    *Chg = REG_ALL;
}



int IsZPName (const char* Name, unsigned short* RegInfo)
/* Return true if the given name is a zero page symbol. If the RegInfo
 * pointer is not NULL, it is filled with the register info for the
 * zero page location found.
 */
{
    unsigned I;
    const ZPInfo* Info;

    /* Because of the low number of symbols, we do a linear search here */
    for (I = 0, Info = ZPInfoTable; I < ZPInfoCount; ++I, ++Info) {
       	if (strncmp (Name, Info->Name, Info->Len) == 0 &&
 	    (Name[Info->Len] == '\0' || Name[Info->Len] == '+')) {
 	    /* Found */
	    if (RegInfo) {
		*RegInfo = Info->RegInfo;
	    }
 	    return 1;
 	}
    }

    /* Not found */
    return 0;
}



static unsigned GetRegInfo1 (CodeSeg* S,
 		      	     CodeEntry* E,
 		   	     int Index,
 		     	     Collection* Visited,
		     	     unsigned Used,
		     	     unsigned Unused);
/* Recursively called subfunction for GetRegInfo. */



static unsigned GetRegInfo2 (CodeSeg* S,
		      	     CodeEntry* E,
		    	     int Index,
		     	     Collection* Visited,
		     	     unsigned Used,
		     	     unsigned Unused)
/* Recursively called subfunction for GetRegInfo. */
{
    /* Follow the instruction flow recording register usage. */
    while (1) {

	unsigned R;

	/* Check if we have already visited the current code entry. If so,
	 * bail out.
	 */
	if (CE_HasMark (E)) {
	    break;
	}

	/* Mark this entry as already visited */
	CE_SetMark (E);
	CollAppend (Visited, E);

	/* Evaluate the used registers */
	R = E->Use;
	if (E->OPC == OP65_RTS ||
	    ((E->Info & OF_BRA) != 0 && E->JumpTo == 0)) {
	    /* This instruction will leave the function */
	    R |= S->ExitRegs;
	}
       	if (R != REG_NONE) {
	    /* We are not interested in the use of any register that has been
	     * used before.
	     */
	    R &= ~Unused;
	    /* Remember the remaining registers */
	    Used |= R;
	}

	/* Evaluate the changed registers */
       	if ((R = E->Chg) != REG_NONE) {
	    /* We are not interested in the use of any register that has been
	     * used before.
	     */
	    R &= ~Used;
	    /* Remember the remaining registers */
	    Unused |= R;
	}

       	/* If we know about all registers now, bail out */
       	if ((Used | Unused) == REG_AXY) {
    	    break;
    	}

	/* If the instruction is an RTS or RTI, we're done */
       	if ((E->Info & OF_RET) != 0) {
	    break;
	}

	/* If we have an unconditional branch, follow this branch if possible,
	 * otherwise we're done.
	 */
	if ((E->Info & OF_UBRA) != 0) {

	    /* Does this jump have a valid target? */
	    if (E->JumpTo) {

	       	/* Unconditional jump */
 	       	E     = E->JumpTo->Owner;
		Index = -1;	  	/* Invalidate */

	    } else {
	       	/* Jump outside means we're done */
	       	break;
	    }

       	/* In case of conditional branches, follow the branch if possible and
	 * follow the normal flow (branch not taken) afterwards. If we cannot
	 * follow the branch, we're done.
	 */
	} else if ((E->Info & OF_CBRA) != 0) {

    	    if (E->JumpTo) {

	       	/* Recursively determine register usage at the branch target */
		unsigned U1;
		unsigned U2;

		U1 = GetRegInfo1 (S, E->JumpTo->Owner, -1, Visited, Used, Unused);
		if (U1 == REG_AXY) {
		    /* All registers used, no need for second call */
		    return REG_AXY;
		}
		if (Index < 0) {
		    Index = CS_GetEntryIndex (S, E);
		}
       	       	if ((E = CS_GetEntry (S, ++Index)) == 0) {
		    Internal ("GetRegInfo2: No next entry!");
		}
		U2 = GetRegInfo1 (S, E, Index, Visited, Used, Unused);
	   	return U1 | U2;	       	/* Used in any of the branches */

	    } else {
	   	/* Jump to global symbol */
	  	break;
	    }

       	} else {

	    /* Just go to the next instruction */
	    if (Index < 0) {
	     	Index = CS_GetEntryIndex (S, E);
	    }
	    E = CS_GetEntry (S, ++Index);
	    if (E == 0) {
	     	/* No next entry */
	     	Internal ("GetRegInfo2: No next entry!");
	    }

	}

    }

    /* Return to the caller the complement of all unused registers */
    return Used;
}



static unsigned GetRegInfo1 (CodeSeg* S,
		      	     CodeEntry* E,
		   	     int Index,
		     	     Collection* Visited,
		     	     unsigned Used,
		     	     unsigned Unused)
/* Recursively called subfunction for GetRegInfo. */
{
    /* Remember the current count of the line collection */
    unsigned Count = CollCount (Visited);

    /* Call the worker routine */
    unsigned R = GetRegInfo2 (S, E, Index, Visited, Used, Unused);

    /* Restore the old count, unmarking all new entries */
    unsigned NewCount = CollCount (Visited);
    while (NewCount-- > Count) {
	CodeEntry* E = CollAt (Visited, NewCount);
	CE_ResetMark (E);
	CollDelete (Visited, NewCount);
    }

    /* Return the registers used */
    return R;
}



unsigned GetRegInfo (struct CodeSeg* S, unsigned Index)
/* Determine register usage information for the instructions starting at the
 * given index.
 */
{
    CodeEntry* 	    E;
    Collection 	    Visited;	/* Visited entries */
    unsigned        R;

    /* Get the code entry for the given index */
    if (Index >= CS_GetEntryCount (S)) {
	/* There is no such code entry */
	return REG_NONE;
    }
    E = CS_GetEntry (S, Index);

    /* Initialize the data structure used to collection information */
    InitCollection (&Visited);

    /* Call the recursive subfunction */
    R = GetRegInfo1 (S, E, Index, &Visited, REG_NONE, REG_NONE);

    /* Delete the line collection */
    DoneCollection (&Visited);

    /* Return the registers used */
    return R;
}



int RegAUsed (struct CodeSeg* S, unsigned Index)
/* Check if the value in A is used. */
{
    return (GetRegInfo (S, Index) & REG_A) != 0;
}



int RegXUsed (struct CodeSeg* S, unsigned Index)
/* Check if the value in X is used. */
{
    return (GetRegInfo (S, Index) & REG_X) != 0;
}



int RegYUsed (struct CodeSeg* S, unsigned Index)
/* Check if the value in Y is used. */
{
    return (GetRegInfo (S, Index) & REG_Y) != 0;
}





