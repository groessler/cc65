/*****************************************************************************/
/*                                                                           */
/*				   codeent.c				     */
/*                                                                           */
/*			      Code segment entry			     */
/*                                                                           */
/*                                                                           */
/*                                                                           */
/* (C) 2001     Ullrich von Bassewitz                                        */
/*              Wacholderweg 14                                              */
/*              D-70597 Stuttgart                                            */
/* EMail:       uz@musoftware.de                                             */
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
#include "chartype.h"
#include "check.h"
#include "xmalloc.h"
#include "xsprintf.h"

/* cc65 */
#include "codeinfo.h"
#include "error.h"
#include "global.h"
#include "codelab.h"
#include "opcodes.h"
#include "codeent.h"



/*****************************************************************************/
/*  	       	    	   	     Data				     */
/*****************************************************************************/



/* Empty argument */
static char EmptyArg[] = "";



/*****************************************************************************/
/*			       Helper functions				     */
/*****************************************************************************/



static void FreeArg (char* Arg)
/* Free a code entry argument */
{
    if (Arg != EmptyArg) {
	xfree (Arg);
    }
}



static char* GetArgCopy (const char* Arg)
/* Create an argument copy for assignment */
{
    if (Arg && Arg[0] != '\0') {
	/* Create a copy */
	return xstrdup (Arg);
    } else {
	/* Use the empty argument string */
	return EmptyArg;
    }
}



static int NumArg (const char* Arg, unsigned long* Num)
/* If the given argument is numerical, convert it and return true. Otherwise
 * set Num to zero and return false.
 */
{
    char* End;
    unsigned long Val;

    /* Determine the base */
    int Base = 10;
    if (*Arg == '$') {
    	++Arg;
    	Base = 16;
    } else if (*Arg == '%') {
    	++Arg;
    	Base = 2;
    }

    /* Convert the value. strtol is not exactly what we want here, but it's
     * cheap and may be replaced by something fancier later.
     */
    Val = strtoul (Arg, &End, Base);

    /* Check if the conversion was successful */
    if (*End != '\0') {

	/* Could not convert */
	*Num = 0;
	return 0;

    } else {

	/* Conversion ok */
	*Num = Val;
	return 1;

    }
}



static void SetUseChgInfo (CodeEntry* E, const OPCDesc* D)
/* Set the Use and Chg in E */
{
    unsigned short Use;

    /* If this is a subroutine call, or a jump to an external function,
     * lookup the information about this function and use it. The jump itself
     * does not change any registers, so we don't need to use the data from D.
     */
    if ((E->Info & (OF_BRA | OF_CALL)) != 0 && E->JumpTo == 0) {
     	/* A subroutine call or jump to external symbol (function exit) */
     	GetFuncInfo (E->Arg, &E->Use, &E->Chg);
    } else {
     	/* Some other instruction. Use the values from the opcode description
	 * plus addressing mode info.
	 */
     	E->Use = D->Use | GetAMUseInfo (E->AM);
	E->Chg = D->Chg;

	/* Check for special zero page registers used */
	switch (E->AM) {

	    case AM65_ZP:
	    case AM65_ABS:
	    /* Be conservative: */
	    case AM65_ZPX:
	    case AM65_ABSX:
	    case AM65_ABSY:
	        if (IsZPName (E->Arg, &Use) && Use != REG_NONE) {
		    if (E->OPC == OP65_INC || E->OPC == OP65_DEC) {
			E->Chg |= Use;
			E->Use |= Use;
		    } else if ((E->Info & OF_STORE) != 0) {
			E->Chg |= Use;
		    } else {
			E->Use |= Use;
		    }
		}
	        break;

	    case AM65_ZPX_IND:
	    case AM65_ZP_INDY:
	    case AM65_ZP_IND:
	        if (IsZPName (E->Arg, &Use) && Use != REG_NONE) {
		    /* These addressing modes will never change the zp loc */
		    E->Use |= Use;
		}
	        break;
	    
	    default:
	        /* Keep gcc silent */
	        break;
	}
    }
}



/*****************************************************************************/
/*     	       	      	   	     Code				     */
/*****************************************************************************/



CodeEntry* NewCodeEntry (opc_t OPC, am_t AM, const char* Arg,
	   		 CodeLabel* JumpTo, LineInfo* LI)
/* Create a new code entry, initialize and return it */
{
    /* Get the opcode description */
    const OPCDesc* D = GetOPCDesc (OPC);

    /* Allocate memory */
    CodeEntry* E = xmalloc (sizeof (CodeEntry));

    /* Initialize the fields */
    E->OPC    = D->OPC;
    E->AM     = AM;
    E->Arg    = GetArgCopy (Arg);
    E->Flags  = NumArg (E->Arg, &E->Num)? CEF_NUMARG : 0;
    E->Info   = D->Info;
    E->Size   = GetInsnSize (E->OPC, E->AM);
    E->JumpTo = JumpTo;
    E->LI     = UseLineInfo (LI);
    E->RI     = 0;
    SetUseChgInfo (E, D);
    InitCollection (&E->Labels);

    /* If we have a label given, add this entry to the label */
    if (JumpTo) {
     	CollAppend (&JumpTo->JumpFrom, E);
    }

    /* Return the initialized struct */
    return E;
}



void FreeCodeEntry (CodeEntry* E)
/* Free the given code entry */
{
    /* Free the string argument if we have one */
    FreeArg (E->Arg);

    /* Cleanup the collection */
    DoneCollection (&E->Labels);

    /* Release the line info */
    ReleaseLineInfo (E->LI);

    /* Delete the register info */
    CE_FreeRegInfo (E);

    /* Free the entry */
    xfree (E);
}



void CE_ReplaceOPC (CodeEntry* E, opc_t OPC)
/* Replace the opcode of the instruction. This will also replace related info,
 * Size, Use and Chg, but it will NOT update any arguments or labels.
 */
{
    /* Get the opcode descriptor */
    const OPCDesc* D = GetOPCDesc (OPC);

    /* Replace the opcode */
    E->OPC  = OPC;
    E->Info = D->Info;
    E->Size = GetInsnSize (E->OPC, E->AM);
    SetUseChgInfo (E, D);
}



int CodeEntriesAreEqual (const CodeEntry* E1, const CodeEntry* E2)
/* Check if both code entries are equal */
{
    return E1->OPC == E2->OPC && E1->AM == E2->AM && strcmp (E1->Arg, E2->Arg) == 0;
}



void CE_AttachLabel (CodeEntry* E, CodeLabel* L)
/* Attach the label to the entry */
{
    /* Add it to the entries label list */
    CollAppend (&E->Labels, L);

    /* Tell the label about it's owner */
    L->Owner = E;
}



void CE_MoveLabel (CodeLabel* L, CodeEntry* E)
/* Move the code label L from it's former owner to the code entry E. */
{
    /* Delete the label from the owner */
    CollDeleteItem (&L->Owner->Labels, L);

    /* Set the new owner */
    CollAppend (&E->Labels, L);
    L->Owner = E;
}



void CE_SetNumArg (CodeEntry* E, long Num)
/* Set a new numeric argument for the given code entry that must already
 * have a numeric argument.
 */
{
    char Buf[16];

    /* Check that the entry has a numerical argument */
    CHECK (E->Flags & CEF_NUMARG);

    /* Make the new argument string */
    if (E->Size == 2) {
	Num &= 0xFF;
    	xsprintf (Buf, sizeof (Buf), "$%02X", (unsigned) Num);
    } else if (E->Size == 3) {
	Num &= 0xFFFF;
    	xsprintf (Buf, sizeof (Buf), "$%04X", (unsigned) Num);
    } else {
    	Internal ("Invalid instruction size in CE_SetNumArg");
    }

    /* Free the old argument */
    FreeArg (E->Arg);

    /* Assign the new one */
    E->Arg = GetArgCopy (Buf);

    /* Use the new numerical value */
    E->Num = Num;
}



int CE_KnownImm (const CodeEntry* E)
/* Return true if the argument of E is a known immediate value */
{
    return (E->AM == AM65_IMM && (E->Flags & CEF_NUMARG) != 0);
}



void CE_FreeRegInfo (CodeEntry* E)
/* Free an existing register info struct */
{
    if (E->RI) {
	FreeRegInfo (E->RI);
	E->RI = 0;
    }
}



void CE_GenRegInfo (CodeEntry* E, RegContents* InputRegs)
/* Generate register info for this instruction. If an old info exists, it is
 * overwritten.
 */
{
    /* Pointers to the register contents */
    RegContents* In;
    RegContents* Out;

    /* Function register usage */
    unsigned short Use, Chg;

    /* If we don't have a register info struct, allocate one. */
    if (E->RI == 0) {
	E->RI = NewRegInfo (InputRegs);
    } else {
	if (InputRegs) {
	    E->RI->In  = *InputRegs;
	} else {
	    RC_Invalidate (&E->RI->In);
	}
       	E->RI->Out2 = E->RI->Out = E->RI->In;
    }

    /* Get pointers to the register contents */
    In  = &E->RI->In;
    Out	= &E->RI->Out;

    /* Handle the different instructions */
    switch (E->OPC) {

	case OP65_ADC:
	    /* We don't know the value of the carry, so the result is
	     * always unknown.
	     */
	    Out->RegA = -1;
	    break;

	case OP65_AND:
       	    if (In->RegA >= 0) {
		if (CE_KnownImm (E)) {
		    Out->RegA = In->RegA & (short) E->Num;
		} else {
		    Out->RegA = -1;
		}
	    }
	    break;

	case OP65_ASL:
	    if (E->AM == AM65_ACC && In->RegA >= 0) {
		Out->RegA = (In->RegA << 1) & 0xFF;
	    }
	    break;

	case OP65_BCC:
	    break;

	case OP65_BCS:
	    break;

	case OP65_BEQ:
	    break;

	case OP65_BIT:
	    break;

	case OP65_BMI:
	    break;

	case OP65_BNE:
	    break;

	case OP65_BPL:
	    break;

	case OP65_BRA:
	    break;

	case OP65_BRK:
	    break;

	case OP65_BVC:
	    break;

	case OP65_BVS:
	    break;

	case OP65_CLC:
	    break;

	case OP65_CLD:
	    break;

	case OP65_CLI:
	    break;

	case OP65_CLV:
	    break;

	case OP65_CMP:
	    break;

	case OP65_CPX:
	    break;

	case OP65_CPY:
	    break;

	case OP65_DEA:
	    if (In->RegA >= 0) {
	    	Out->RegA = In->RegA - 1;
	    }
	    break;

	case OP65_DEC:
	    if (E->AM == AM65_ACC && In->RegA >= 0) {
	    	Out->RegA = In->RegA - 1;
	    }
	    break;

	case OP65_DEX:
       	    if (In->RegX >= 0) {
	    	Out->RegX = In->RegX - 1;
	    }
	    break;

	case OP65_DEY:
       	    if (In->RegY >= 0) {
	    	Out->RegY = In->RegY - 1;
	    }
	    break;

	case OP65_EOR:
       	    if (In->RegA >= 0) {
		if (CE_KnownImm (E)) {
		    Out->RegA = In->RegA ^ (short) E->Num;
		} else {
		    Out->RegA = -1;
		}
	    }
	    break;

	case OP65_INA:
	    if (In->RegA >= 0) {
		Out->RegA = In->RegA + 1;
	    }
	    break;

	case OP65_INC:
	    if (E->AM == AM65_ACC && In->RegA >= 0) {
		Out->RegA = In->RegA + 1;
	    }
	    break;

	case OP65_INX:
	    if (In->RegX >= 0) {
		Out->RegX = In->RegX + 1;
	    }
	    break;

	case OP65_INY:
	    if (In->RegY >= 0) {
		Out->RegY = In->RegY + 1;
	    }
	    break;

	case OP65_JCC:
	    break;

	case OP65_JCS:
	    break;

	case OP65_JEQ:
	    break;

	case OP65_JMI:
	    break;

	case OP65_JMP:
	    break;

	case OP65_JNE:
	    break;

	case OP65_JPL:
	    break;

	case OP65_JSR:
	    /* Get the code info for the function */
	    GetFuncInfo (E->Arg, &Use, &Chg);
	    if (Chg & REG_A) {
		Out->RegA = -1;
	    }
	    if (Chg & REG_X) {
		Out->RegX = -1;
	    }
	    if (Chg & REG_Y) {
		Out->RegY = -1;
	    }
	    break;

	case OP65_JVC:
	    break;

	case OP65_JVS:
	    break;

	case OP65_LDA:
	    if (CE_KnownImm (E)) {
		Out->RegA = (unsigned char) E->Num;
	    } else {
		/* A is now unknown */
		Out->RegA = -1;
	    }
	    break;

	case OP65_LDX:
	    if (CE_KnownImm (E)) {
		Out->RegX = (unsigned char) E->Num;
	    } else {
		/* X is now unknown */
		Out->RegX = -1;
	    }
	    break;

	case OP65_LDY:
	    if (CE_KnownImm (E)) {
		Out->RegY = (unsigned char) E->Num;
	    } else {
		/* Y is now unknown */
		Out->RegY = -1;
	    }
	    break;

	case OP65_LSR:
	    if (E->AM == AM65_ACC && In->RegA >= 0) {
		Out->RegA = (In->RegA >> 1) & 0xFF;
	    }
	    break;

	case OP65_NOP:
	    break;

	case OP65_ORA:
	    if (In->RegA >= 0) {
		if (CE_KnownImm (E)) {
		    Out->RegA = In->RegA | (short) E->Num;
		} else {
		    /* A is now unknown */
		    Out->RegA = -1;
		}
	    }
	    break;

	case OP65_PHA:
	    break;

	case OP65_PHP:
	    break;

	case OP65_PHX:
	    break;

	case OP65_PHY:
	    break;

	case OP65_PLA:
	    Out->RegA = -1;
	    break;

	case OP65_PLP:
	    break;

	case OP65_PLX:
	    Out->RegX = -1;
	    break;

	case OP65_PLY:
	    Out->RegY = -1;
	    break;

	case OP65_ROL:
	    Out->RegA = -1;
	    break;

	case OP65_ROR:
	    Out->RegA = -1;
	    break;

	case OP65_RTI:
	    break;

	case OP65_RTS:
	    break;

	case OP65_SBC:
	    /* We don't know the value of the carry bit */
	    Out->RegA = -1;
	    break;

	case OP65_SEC:
	    break;

	case OP65_SED:
	    break;

	case OP65_SEI:
	    break;

	case OP65_STA:
	    break;

	case OP65_STX:
	    break;

	case OP65_STY:
	    break;

	case OP65_TAX:
	    Out->RegX = In->RegA;
	    break;

	case OP65_TAY:
	    Out->RegY = In->RegA;
	    break;

	case OP65_TRB:
	    /* For now... */
	    Out->RegA = -1;
	    break;

	case OP65_TSB:
	    /* For now... */
	    Out->RegA = -1;
	    break;

	case OP65_TSX:
	    Out->RegX = -1;
	    break;

	case OP65_TXA:
	    Out->RegA = In->RegX;
	    break;

	case OP65_TXS:
	    break;

	case OP65_TYA:
	    Out->RegA = In->RegY;
	    break;

	default:
	    break;

    }
}



static char* RegInfoDesc (unsigned U, char* Buf)
/* Return a string containing register info */
{
    Buf[0] = '\0';
    if (U & REG_SREG) {
    	strcat (Buf, "E");
    }
    if (U & REG_A) {
	strcat (Buf, "A");
    }
    if (U & REG_X) {
    	strcat (Buf, "X");
    }
    if (U & REG_Y) {
    	strcat (Buf, "Y");
    }
    if (U & REG_TMP1) {
    	strcat (Buf, "T1");
    }
    if (U & REG_TMP2) {
    	strcat (Buf, "T2");
    }
    if (U & REG_TMP3) {
    	strcat (Buf, "T3");
    }
    if (U & REG_PTR1) {
    	strcat (Buf, "P1");
    }
    if (U & REG_PTR2) {
    	strcat (Buf, "P2");
    }
    if (U & REG_PTR3) {
    	strcat (Buf, "P3");
    }
    if (U & REG_PTR4) {
    	strcat (Buf, "P4");
    }
    return Buf;
}



void CE_Output (const CodeEntry* E, FILE* F)
/* Output the code entry to a file */
{
    const OPCDesc* D;
    unsigned Chars;
    const char* Target;

    /* If we have a label, print that */
    unsigned LabelCount = CollCount (&E->Labels);
    unsigned I;
    for (I = 0; I < LabelCount; ++I) {
    	CL_Output (CollConstAt (&E->Labels, I), F);
    }

    /* Get the opcode description */
    D = GetOPCDesc (E->OPC);

    /* Print the mnemonic */
    Chars = fprintf (F, "\t%s", D->Mnemo);

    /* Print the operand */
    switch (E->AM) {

	case AM_IMP:
    	case AM65_IMP:
    	    /* implicit */
    	    break;

    	case AM65_ACC:
    	    /* accumulator */
    	    Chars += fprintf (F, "%*sa", 9-Chars, "");
    	    break;

	case AM_IMM:
	case AM65_IMM:
    	    /* immidiate */
    	    Chars += fprintf (F, "%*s#%s", 9-Chars, "", E->Arg);
    	    break;

	case AM_ABS:
    	case AM65_ZP:
    	case AM65_ABS:
	    /* zeropage and absolute */
	    Chars += fprintf (F, "%*s%s", 9-Chars, "", E->Arg);
       	    break;

	case AM65_ZPX:
	case AM65_ABSX:
	    /* zeropage,X and absolute,X */
	    Chars += fprintf (F, "%*s%s,x", 9-Chars, "", E->Arg);
	    break;

	case AM65_ABSY:
	    /* absolute,Y */
	    Chars += fprintf (F, "%*s%s,y", 9-Chars, "", E->Arg);
	    break;

	case AM65_ZPX_IND:
	    /* (zeropage,x) */
       	    Chars += fprintf (F, "%*s(%s,x)", 9-Chars, "", E->Arg);
	    break;

	case AM65_ZP_INDY:
	    /* (zeropage),y */
       	    Chars += fprintf (F, "%*s(%s),y", 9-Chars, "", E->Arg);
	    break;

	case AM65_ZP_IND:
	    /* (zeropage) */
       	    Chars += fprintf (F, "%*s(%s)", 9-Chars, "", E->Arg);
	    break;

	case AM65_BRA:
	    /* branch */
	    Target = E->JumpTo? E->JumpTo->Name : E->Arg;
	    Chars += fprintf (F, "%*s%s", 9-Chars, "", Target);
	    break;

	default:
	    Internal ("Invalid addressing mode");

    }

    /* Print usage info if requested by the debugging flag */
    if (Debug) {
	char Use [128];
	char Chg [128];
       	fprintf (F,
       	       	 "%*s; USE: %-18s CHG: %-18s SIZE: %u\n",
       	       	 30-Chars, "",
		 RegInfoDesc (E->Use, Use),
		 RegInfoDesc (E->Chg, Chg),
	       	 E->Size);
    } else {
	/* Terminate the line */
	fprintf (F, "\n");
    }
}






