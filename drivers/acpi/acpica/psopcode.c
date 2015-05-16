/*                                                                             
  
                                                                      
  
                                                                             */

/*
 * Copyright (C) 2000 - 2012, Intel Corp.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions, and the following disclaimer,
 *    without modification.
 * 2. Redistributions in binary form must reproduce at minimum a disclaimer
 *    substantially similar to the "NO WARRANTY" disclaimer below
 *    ("Disclaimer") and any redistribution must be conditioned upon
 *    including a substantially similar Disclaimer requirement for further
 *    binary redistribution.
 * 3. Neither the names of the above-listed copyright holders nor the names
 *    of any contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * NO WARRANTY
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 */

#include <acpi/acpi.h>
#include "accommon.h"
#include "acparser.h"
#include "acopcode.h"
#include "amlcode.h"

#define _COMPONENT          ACPI_PARSER
ACPI_MODULE_NAME("psopcode")

static const u8 acpi_gbl_argument_count[] =
    { 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 6 };

/*                                                                              
  
                                    
  
                                                                                
                                                                              
                                                                               
                                 
  
                                                                              */

/*
                                
  

                                                                   

             
              
                    
              
                 
                 
             
                   
                  
            
             
             
             
              
                    
                        
                         
                         
                          
                          
                      
                      
                    

                                                          

             
              
                    
              
                 
                 
             
                   
                  
            
             
             
             
              
                      

                                                                  

             
              
                    
              
                 
                 
            
             
             
             
              
                    
                        
                         
                         
                          
                          
                      
                      
                    

                                                         

             
              
                    
              
                 
                 
            
             
             
             
              
                      

                                                                    
                               

              
                   
                    
                        
                         
                         
                          
                          
              
              

               

                    
             
                   
                  

                        

                    
                        
                         
                         
                          
                          

                                                                              */

/*
                                                                               
                            
 */
const struct acpi_opcode_info acpi_gbl_aml_op_info[AML_NUM_OPCODES] = {
/*                                     */
/*                                                                                                                                                                                     */

/*    */ ACPI_OP("Zero", ARGP_ZERO_OP, ARGI_ZERO_OP, ACPI_TYPE_INTEGER,
		 AML_CLASS_ARGUMENT, AML_TYPE_CONSTANT, AML_CONSTANT),
/*    */ ACPI_OP("One", ARGP_ONE_OP, ARGI_ONE_OP, ACPI_TYPE_INTEGER,
		 AML_CLASS_ARGUMENT, AML_TYPE_CONSTANT, AML_CONSTANT),
/*    */ ACPI_OP("Alias", ARGP_ALIAS_OP, ARGI_ALIAS_OP,
		 ACPI_TYPE_LOCAL_ALIAS, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_SIMPLE,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("Name", ARGP_NAME_OP, ARGI_NAME_OP, ACPI_TYPE_ANY,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_COMPLEX,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("ByteConst", ARGP_BYTE_OP, ARGI_BYTE_OP,
		 ACPI_TYPE_INTEGER, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_CONSTANT),
/*    */ ACPI_OP("WordConst", ARGP_WORD_OP, ARGI_WORD_OP,
		 ACPI_TYPE_INTEGER, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_CONSTANT),
/*    */ ACPI_OP("DwordConst", ARGP_DWORD_OP, ARGI_DWORD_OP,
		 ACPI_TYPE_INTEGER, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_CONSTANT),
/*    */ ACPI_OP("String", ARGP_STRING_OP, ARGI_STRING_OP,
		 ACPI_TYPE_STRING, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_CONSTANT),
/*    */ ACPI_OP("Scope", ARGP_SCOPE_OP, ARGI_SCOPE_OP,
		 ACPI_TYPE_LOCAL_SCOPE, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_NO_OBJ,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("Buffer", ARGP_BUFFER_OP, ARGI_BUFFER_OP,
		 ACPI_TYPE_BUFFER, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_OBJECT,
		 AML_HAS_ARGS | AML_DEFER | AML_CONSTANT),
/*    */ ACPI_OP("Package", ARGP_PACKAGE_OP, ARGI_PACKAGE_OP,
		 ACPI_TYPE_PACKAGE, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_OBJECT,
		 AML_HAS_ARGS | AML_DEFER | AML_CONSTANT),
/*    */ ACPI_OP("Method", ARGP_METHOD_OP, ARGI_METHOD_OP,
		 ACPI_TYPE_METHOD, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_COMPLEX,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED | AML_DEFER),
/*    */ ACPI_OP("Local0", ARGP_LOCAL0, ARGI_LOCAL0,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local1", ARGP_LOCAL1, ARGI_LOCAL1,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local2", ARGP_LOCAL2, ARGI_LOCAL2,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local3", ARGP_LOCAL3, ARGI_LOCAL3,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local4", ARGP_LOCAL4, ARGI_LOCAL4,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local5", ARGP_LOCAL5, ARGI_LOCAL5,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local6", ARGP_LOCAL6, ARGI_LOCAL6,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Local7", ARGP_LOCAL7, ARGI_LOCAL7,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LOCAL_VARIABLE, 0),
/*    */ ACPI_OP("Arg0", ARGP_ARG0, ARGI_ARG0,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg1", ARGP_ARG1, ARGI_ARG1,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg2", ARGP_ARG2, ARGI_ARG2,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg3", ARGP_ARG3, ARGI_ARG3,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg4", ARGP_ARG4, ARGI_ARG4,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg5", ARGP_ARG5, ARGI_ARG5,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Arg6", ARGP_ARG6, ARGI_ARG6,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_METHOD_ARGUMENT, 0),
/*    */ ACPI_OP("Store", ARGP_STORE_OP, ARGI_STORE_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R),
/*    */ ACPI_OP("RefOf", ARGP_REF_OF_OP, ARGI_REF_OF_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R),
/*    */ ACPI_OP("Add", ARGP_ADD_OP, ARGI_ADD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("Concatenate", ARGP_CONCAT_OP, ARGI_CONCAT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Subtract", ARGP_SUBTRACT_OP, ARGI_SUBTRACT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("Increment", ARGP_INCREMENT_OP, ARGI_INCREMENT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Decrement", ARGP_DECREMENT_OP, ARGI_DECREMENT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Multiply", ARGP_MULTIPLY_OP, ARGI_MULTIPLY_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("Divide", ARGP_DIVIDE_OP, ARGI_DIVIDE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_2T_1R,
		 AML_FLAGS_EXEC_2A_2T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ShiftLeft", ARGP_SHIFT_LEFT_OP, ARGI_SHIFT_LEFT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("ShiftRight", ARGP_SHIFT_RIGHT_OP, ARGI_SHIFT_RIGHT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("And", ARGP_BIT_AND_OP, ARGI_BIT_AND_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("NAnd", ARGP_BIT_NAND_OP, ARGI_BIT_NAND_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("Or", ARGP_BIT_OR_OP, ARGI_BIT_OR_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("NOr", ARGP_BIT_NOR_OP, ARGI_BIT_NOR_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("XOr", ARGP_BIT_XOR_OP, ARGI_BIT_XOR_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_MATH | AML_CONSTANT),
/*    */ ACPI_OP("Not", ARGP_BIT_NOT_OP, ARGI_BIT_NOT_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("FindSetLeftBit", ARGP_FIND_SET_LEFT_BIT_OP,
		 ARGI_FIND_SET_LEFT_BIT_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("FindSetRightBit", ARGP_FIND_SET_RIGHT_BIT_OP,
		 ARGI_FIND_SET_RIGHT_BIT_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("DerefOf", ARGP_DEREF_OF_OP, ARGI_DEREF_OF_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_1R, AML_FLAGS_EXEC_1A_0T_1R),
/*    */ ACPI_OP("Notify", ARGP_NOTIFY_OP, ARGI_NOTIFY_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_0T_0R, AML_FLAGS_EXEC_2A_0T_0R),
/*    */ ACPI_OP("SizeOf", ARGP_SIZE_OF_OP, ARGI_SIZE_OF_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R | AML_NO_OPERAND_RESOLVE),
/*    */ ACPI_OP("Index", ARGP_INDEX_OP, ARGI_INDEX_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R),
/*    */ ACPI_OP("Match", ARGP_MATCH_OP, ARGI_MATCH_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_6A_0T_1R,
		 AML_FLAGS_EXEC_6A_0T_1R | AML_CONSTANT),
/*    */ ACPI_OP("CreateDWordField", ARGP_CREATE_DWORD_FIELD_OP,
		 ARGI_CREATE_DWORD_FIELD_OP,
		 ACPI_TYPE_BUFFER_FIELD, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_CREATE),
/*    */ ACPI_OP("CreateWordField", ARGP_CREATE_WORD_FIELD_OP,
		 ARGI_CREATE_WORD_FIELD_OP,
		 ACPI_TYPE_BUFFER_FIELD, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_CREATE),
/*    */ ACPI_OP("CreateByteField", ARGP_CREATE_BYTE_FIELD_OP,
		 ARGI_CREATE_BYTE_FIELD_OP,
		 ACPI_TYPE_BUFFER_FIELD, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_CREATE),
/*    */ ACPI_OP("CreateBitField", ARGP_CREATE_BIT_FIELD_OP,
		 ARGI_CREATE_BIT_FIELD_OP,
		 ACPI_TYPE_BUFFER_FIELD, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_CREATE),
/*    */ ACPI_OP("ObjectType", ARGP_TYPE_OP, ARGI_TYPE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R | AML_NO_OPERAND_RESOLVE),
/*    */ ACPI_OP("LAnd", ARGP_LAND_OP, ARGI_LAND_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R | AML_LOGICAL_NUMERIC | AML_CONSTANT),
/*    */ ACPI_OP("LOr", ARGP_LOR_OP, ARGI_LOR_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R | AML_LOGICAL_NUMERIC | AML_CONSTANT),
/*    */ ACPI_OP("LNot", ARGP_LNOT_OP, ARGI_LNOT_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_0T_1R,
		 AML_FLAGS_EXEC_1A_0T_1R | AML_CONSTANT),
/*    */ ACPI_OP("LEqual", ARGP_LEQUAL_OP, ARGI_LEQUAL_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R | AML_LOGICAL | AML_CONSTANT),
/*    */ ACPI_OP("LGreater", ARGP_LGREATER_OP, ARGI_LGREATER_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R | AML_LOGICAL | AML_CONSTANT),
/*    */ ACPI_OP("LLess", ARGP_LLESS_OP, ARGI_LLESS_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R | AML_LOGICAL | AML_CONSTANT),
/*    */ ACPI_OP("If", ARGP_IF_OP, ARGI_IF_OP, ACPI_TYPE_ANY,
		 AML_CLASS_CONTROL, AML_TYPE_CONTROL, AML_HAS_ARGS),
/*    */ ACPI_OP("Else", ARGP_ELSE_OP, ARGI_ELSE_OP, ACPI_TYPE_ANY,
		 AML_CLASS_CONTROL, AML_TYPE_CONTROL, AML_HAS_ARGS),
/*    */ ACPI_OP("While", ARGP_WHILE_OP, ARGI_WHILE_OP, ACPI_TYPE_ANY,
		 AML_CLASS_CONTROL, AML_TYPE_CONTROL, AML_HAS_ARGS),
/*    */ ACPI_OP("Noop", ARGP_NOOP_OP, ARGI_NOOP_OP, ACPI_TYPE_ANY,
		 AML_CLASS_CONTROL, AML_TYPE_CONTROL, 0),
/*    */ ACPI_OP("Return", ARGP_RETURN_OP, ARGI_RETURN_OP,
		 ACPI_TYPE_ANY, AML_CLASS_CONTROL,
		 AML_TYPE_CONTROL, AML_HAS_ARGS),
/*    */ ACPI_OP("Break", ARGP_BREAK_OP, ARGI_BREAK_OP, ACPI_TYPE_ANY,
		 AML_CLASS_CONTROL, AML_TYPE_CONTROL, 0),
/*    */ ACPI_OP("BreakPoint", ARGP_BREAK_POINT_OP, ARGI_BREAK_POINT_OP,
		 ACPI_TYPE_ANY, AML_CLASS_CONTROL, AML_TYPE_CONTROL, 0),
/*    */ ACPI_OP("Ones", ARGP_ONES_OP, ARGI_ONES_OP, ACPI_TYPE_INTEGER,
		 AML_CLASS_ARGUMENT, AML_TYPE_CONSTANT, AML_CONSTANT),

/*                                                      */

/*    */ ACPI_OP("Mutex", ARGP_MUTEX_OP, ARGI_MUTEX_OP, ACPI_TYPE_MUTEX,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_SIMPLE,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("Event", ARGP_EVENT_OP, ARGI_EVENT_OP, ACPI_TYPE_EVENT,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_SIMPLE,
		 AML_NSOBJECT | AML_NSOPCODE | AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("CondRefOf", ARGP_COND_REF_OF_OP, ARGI_COND_REF_OF_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R, AML_FLAGS_EXEC_1A_1T_1R),
/*    */ ACPI_OP("CreateField", ARGP_CREATE_FIELD_OP,
		 ARGI_CREATE_FIELD_OP, ACPI_TYPE_BUFFER_FIELD,
		 AML_CLASS_CREATE, AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_FIELD | AML_CREATE),
/*    */ ACPI_OP("Load", ARGP_LOAD_OP, ARGI_LOAD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_0R,
		 AML_FLAGS_EXEC_1A_1T_0R),
/*    */ ACPI_OP("Stall", ARGP_STALL_OP, ARGI_STALL_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_0T_0R,
		 AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("Sleep", ARGP_SLEEP_OP, ARGI_SLEEP_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_0T_0R,
		 AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("Acquire", ARGP_ACQUIRE_OP, ARGI_ACQUIRE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_0T_1R, AML_FLAGS_EXEC_2A_0T_1R),
/*    */ ACPI_OP("Signal", ARGP_SIGNAL_OP, ARGI_SIGNAL_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_0R, AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("Wait", ARGP_WAIT_OP, ARGI_WAIT_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_0T_1R,
		 AML_FLAGS_EXEC_2A_0T_1R),
/*    */ ACPI_OP("Reset", ARGP_RESET_OP, ARGI_RESET_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_0T_0R,
		 AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("Release", ARGP_RELEASE_OP, ARGI_RELEASE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_0R, AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("FromBCD", ARGP_FROM_BCD_OP, ARGI_FROM_BCD_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ToBCD", ARGP_TO_BCD_OP, ARGI_TO_BCD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Unload", ARGP_UNLOAD_OP, ARGI_UNLOAD_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_0T_0R, AML_FLAGS_EXEC_1A_0T_0R),
/*    */ ACPI_OP("Revision", ARGP_REVISION_OP, ARGI_REVISION_OP,
		 ACPI_TYPE_INTEGER, AML_CLASS_ARGUMENT,
		 AML_TYPE_CONSTANT, 0),
/*    */ ACPI_OP("Debug", ARGP_DEBUG_OP, ARGI_DEBUG_OP,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_CONSTANT, 0),
/*    */ ACPI_OP("Fatal", ARGP_FATAL_OP, ARGI_FATAL_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_3A_0T_0R,
		 AML_FLAGS_EXEC_3A_0T_0R),
/*    */ ACPI_OP("OperationRegion", ARGP_REGION_OP, ARGI_REGION_OP,
		 ACPI_TYPE_REGION, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_COMPLEX,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED | AML_DEFER),
/*    */ ACPI_OP("Field", ARGP_FIELD_OP, ARGI_FIELD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE | AML_FIELD),
/*    */ ACPI_OP("Device", ARGP_DEVICE_OP, ARGI_DEVICE_OP,
		 ACPI_TYPE_DEVICE, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_NO_OBJ,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("Processor", ARGP_PROCESSOR_OP, ARGI_PROCESSOR_OP,
		 ACPI_TYPE_PROCESSOR, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_SIMPLE,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("PowerResource", ARGP_POWER_RES_OP, ARGI_POWER_RES_OP,
		 ACPI_TYPE_POWER, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_SIMPLE,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("ThermalZone", ARGP_THERMAL_ZONE_OP,
		 ARGI_THERMAL_ZONE_OP, ACPI_TYPE_THERMAL,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_NO_OBJ,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("IndexField", ARGP_INDEX_FIELD_OP, ARGI_INDEX_FIELD_OP,
		 ACPI_TYPE_ANY, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE | AML_FIELD),
/*    */ ACPI_OP("BankField", ARGP_BANK_FIELD_OP, ARGI_BANK_FIELD_OP,
		 ACPI_TYPE_LOCAL_BANK_FIELD, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE | AML_FIELD |
		 AML_DEFER),

/*                                                  */

/*    */ ACPI_OP("LNotEqual", ARGP_LNOTEQUAL_OP, ARGI_LNOTEQUAL_OP,
		 ACPI_TYPE_ANY, AML_CLASS_INTERNAL,
		 AML_TYPE_BOGUS, AML_HAS_ARGS | AML_CONSTANT),
/*    */ ACPI_OP("LLessEqual", ARGP_LLESSEQUAL_OP, ARGI_LLESSEQUAL_OP,
		 ACPI_TYPE_ANY, AML_CLASS_INTERNAL,
		 AML_TYPE_BOGUS, AML_HAS_ARGS | AML_CONSTANT),
/*    */ ACPI_OP("LGreaterEqual", ARGP_LGREATEREQUAL_OP,
		 ARGI_LGREATEREQUAL_OP, ACPI_TYPE_ANY,
		 AML_CLASS_INTERNAL, AML_TYPE_BOGUS,
		 AML_HAS_ARGS | AML_CONSTANT),
/*    */ ACPI_OP("-NamePath-", ARGP_NAMEPATH_OP, ARGI_NAMEPATH_OP,
		 ACPI_TYPE_LOCAL_REFERENCE, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_NSOBJECT | AML_NSNODE),
/*    */ ACPI_OP("-MethodCall-", ARGP_METHODCALL_OP, ARGI_METHODCALL_OP,
		 ACPI_TYPE_METHOD, AML_CLASS_METHOD_CALL,
		 AML_TYPE_METHOD_CALL,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE),
/*    */ ACPI_OP("-ByteList-", ARGP_BYTELIST_OP, ARGI_BYTELIST_OP,
		 ACPI_TYPE_ANY, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, 0),
/*    */ ACPI_OP("-ReservedField-", ARGP_RESERVEDFIELD_OP,
		 ARGI_RESERVEDFIELD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_INTERNAL, AML_TYPE_BOGUS, 0),
/*    */ ACPI_OP("-NamedField-", ARGP_NAMEDFIELD_OP, ARGI_NAMEDFIELD_OP,
		 ACPI_TYPE_ANY, AML_CLASS_INTERNAL,
		 AML_TYPE_BOGUS,
		 AML_NSOBJECT | AML_NSOPCODE | AML_NSNODE | AML_NAMED),
/*    */ ACPI_OP("-AccessField-", ARGP_ACCESSFIELD_OP,
		 ARGI_ACCESSFIELD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_INTERNAL, AML_TYPE_BOGUS, 0),
/*    */ ACPI_OP("-StaticString", ARGP_STATICSTRING_OP,
		 ARGI_STATICSTRING_OP, ACPI_TYPE_ANY,
		 AML_CLASS_INTERNAL, AML_TYPE_BOGUS, 0),
/*    */ ACPI_OP("-Return Value-", ARG_NONE, ARG_NONE, ACPI_TYPE_ANY,
		 AML_CLASS_RETURN_VALUE, AML_TYPE_RETURN,
		 AML_HAS_ARGS | AML_HAS_RETVAL),
/*    */ ACPI_OP("-UNKNOWN_OP-", ARG_NONE, ARG_NONE, ACPI_TYPE_INVALID,
		 AML_CLASS_UNKNOWN, AML_TYPE_BOGUS, AML_HAS_ARGS),
/*    */ ACPI_OP("-ASCII_ONLY-", ARG_NONE, ARG_NONE, ACPI_TYPE_ANY,
		 AML_CLASS_ASCII, AML_TYPE_BOGUS, AML_HAS_ARGS),
/*    */ ACPI_OP("-PREFIX_ONLY-", ARG_NONE, ARG_NONE, ACPI_TYPE_ANY,
		 AML_CLASS_PREFIX, AML_TYPE_BOGUS, AML_HAS_ARGS),

/*                  */

/*    */ ACPI_OP("QwordConst", ARGP_QWORD_OP, ARGI_QWORD_OP,
		 ACPI_TYPE_INTEGER, AML_CLASS_ARGUMENT,
		 AML_TYPE_LITERAL, AML_CONSTANT),
	/*    */ ACPI_OP("Package", /*     */ ARGP_VAR_PACKAGE_OP,
			 ARGI_VAR_PACKAGE_OP, ACPI_TYPE_PACKAGE,
			 AML_CLASS_CREATE, AML_TYPE_CREATE_OBJECT,
			 AML_HAS_ARGS | AML_DEFER),
/*    */ ACPI_OP("ConcatenateResTemplate", ARGP_CONCAT_RES_OP,
		 ARGI_CONCAT_RES_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Mod", ARGP_MOD_OP, ARGI_MOD_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("CreateQWordField", ARGP_CREATE_QWORD_FIELD_OP,
		 ARGI_CREATE_QWORD_FIELD_OP,
		 ACPI_TYPE_BUFFER_FIELD, AML_CLASS_CREATE,
		 AML_TYPE_CREATE_FIELD,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSNODE |
		 AML_DEFER | AML_CREATE),
/*    */ ACPI_OP("ToBuffer", ARGP_TO_BUFFER_OP, ARGI_TO_BUFFER_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ToDecimalString", ARGP_TO_DEC_STR_OP,
		 ARGI_TO_DEC_STR_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ToHexString", ARGP_TO_HEX_STR_OP, ARGI_TO_HEX_STR_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ToInteger", ARGP_TO_INTEGER_OP, ARGI_TO_INTEGER_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R,
		 AML_FLAGS_EXEC_1A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("ToString", ARGP_TO_STRING_OP, ARGI_TO_STRING_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_2A_1T_1R,
		 AML_FLAGS_EXEC_2A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("CopyObject", ARGP_COPY_OP, ARGI_COPY_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_1A_1T_1R, AML_FLAGS_EXEC_1A_1T_1R),
/*    */ ACPI_OP("Mid", ARGP_MID_OP, ARGI_MID_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_3A_1T_1R,
		 AML_FLAGS_EXEC_3A_1T_1R | AML_CONSTANT),
/*    */ ACPI_OP("Continue", ARGP_CONTINUE_OP, ARGI_CONTINUE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_CONTROL, AML_TYPE_CONTROL, 0),
/*    */ ACPI_OP("LoadTable", ARGP_LOAD_TABLE_OP, ARGI_LOAD_TABLE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_EXECUTE,
		 AML_TYPE_EXEC_6A_0T_1R, AML_FLAGS_EXEC_6A_0T_1R),
/*    */ ACPI_OP("DataTableRegion", ARGP_DATA_REGION_OP,
		 ARGI_DATA_REGION_OP, ACPI_TYPE_REGION,
		 AML_CLASS_NAMED_OBJECT, AML_TYPE_NAMED_COMPLEX,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE |
		 AML_NSNODE | AML_NAMED | AML_DEFER),
/*    */ ACPI_OP("[EvalSubTree]", ARGP_SCOPE_OP, ARGI_SCOPE_OP,
		 ACPI_TYPE_ANY, AML_CLASS_NAMED_OBJECT,
		 AML_TYPE_NAMED_NO_OBJ,
		 AML_HAS_ARGS | AML_NSOBJECT | AML_NSOPCODE | AML_NSNODE),

/*                  */

/*    */ ACPI_OP("Timer", ARGP_TIMER_OP, ARGI_TIMER_OP, ACPI_TYPE_ANY,
		 AML_CLASS_EXECUTE, AML_TYPE_EXEC_0A_0T_1R,
			 AML_FLAGS_EXEC_0A_0T_1R),

/*                  */

/*    */ ACPI_OP("-ConnectField-", ARGP_CONNECTFIELD_OP,
			 ARGI_CONNECTFIELD_OP, ACPI_TYPE_ANY,
			 AML_CLASS_INTERNAL, AML_TYPE_BOGUS, AML_HAS_ARGS),
/*    */ ACPI_OP("-ExtAccessField-", ARGP_CONNECTFIELD_OP,
			 ARGI_CONNECTFIELD_OP, ACPI_TYPE_ANY,
			 AML_CLASS_INTERNAL, AML_TYPE_BOGUS, 0)

/*                                    */
};

/*
                                                                
                             
 */
static const u8 acpi_gbl_short_op_index[256] = {
/*                                                           */
/*                                                           */
/*      */ 0x00, 0x01, _UNK, _UNK, _UNK, _UNK, 0x02, _UNK,
/*      */ 0x03, _UNK, 0x04, 0x05, 0x06, 0x07, 0x6E, _UNK,
/*      */ 0x08, 0x09, 0x0a, 0x6F, 0x0b, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, 0x63, _PFX, _PFX,
/*      */ 0x67, 0x66, 0x68, 0x65, 0x69, 0x64, 0x6A, 0x7D,
/*      */ 0x7F, 0x80, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/*      */ _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/*      */ _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC, _ASC,
/*      */ _ASC, _ASC, _ASC, _UNK, _PFX, _UNK, _PFX, _ASC,
/*      */ 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13,
/*      */ 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, _UNK,
/*      */ 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 0x20, 0x21, 0x22,
/*      */ 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a,
/*      */ 0x2b, 0x2c, 0x2d, 0x2e, 0x70, 0x71, 0x2f, 0x30,
/*      */ 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x72,
/*      */ 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x73, 0x74,
/*      */ 0x75, 0x76, _UNK, _UNK, 0x77, 0x78, 0x79, 0x7A,
/*      */ 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x60, 0x61,
/*      */ 0x62, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, 0x44, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x45,
};

/*
                                                                    
                                                                          
 */
static const u8 acpi_gbl_long_op_index[NUM_EXTENDED_OPCODE] = {
/*                                                           */
/*                                                           */
/*      */ _UNK, 0x46, 0x47, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, 0x48, 0x49, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, 0x7B,
/*      */ 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0x50, 0x51,
/*      */ 0x52, 0x53, 0x54, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ 0x55, 0x56, 0x57, 0x7e, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK, _UNK,
/*      */ 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
/*      */ 0x7C,
};

/*                                                                              
  
                                       
  
                                                    
  
                                                       
  
                                                                
                                                                        
  
                                                                              */

const struct acpi_opcode_info *acpi_ps_get_opcode_info(u16 opcode)
{
	ACPI_FUNCTION_NAME(ps_get_opcode_info);

	/*
                                                        
  */
	if (!(opcode & 0xFF00)) {

		/*                                                         */

		return (&acpi_gbl_aml_op_info
			[acpi_gbl_short_op_index[(u8) opcode]]);
	}

	if (((opcode & 0xFF00) == AML_EXTENDED_OPCODE) &&
	    (((u8) opcode) <= MAX_EXTENDED_OPCODE)) {

		/*                                */

		return (&acpi_gbl_aml_op_info
			[acpi_gbl_long_op_index[(u8) opcode]]);
	}

	/*                    */

	ACPI_DEBUG_PRINT((ACPI_DB_EXEC,
			  "Unknown AML opcode [%4.4X]\n", opcode));

	return (&acpi_gbl_aml_op_info[_UNK]);
}

/*                                                                              
  
                                       
  
                                                    
  
                                                                  
                                         
  
                                                                
  
                                                                              */

char *acpi_ps_get_opcode_name(u16 opcode)
{
#if defined(ACPI_DISASSEMBLER) || defined (ACPI_DEBUG_OUTPUT)

	const struct acpi_opcode_info *op;

	op = acpi_ps_get_opcode_info(opcode);

	/*                                             */

	return (op->name);

#else
	return ("OpcodeName unavailable");

#endif
}

/*                                                                              
  
                                          
  
                                                                         
  
                              
  
                                                                         
  
                                                                              */

u8 acpi_ps_get_argument_count(u32 op_type)
{

	if (op_type <= AML_TYPE_EXEC_6A_0T_1R) {
		return (acpi_gbl_argument_count[op_type]);
	}

	return (0);
}