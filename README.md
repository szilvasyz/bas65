<<<<<<< HEAD
# bas65 - BASIC interpreter for 6502 written in C

## Overview
=======
# bas65
BASIC interpreter for 6502 written in C
>>>>>>> f65d8784c6c7f4379ee3e7000857b306351e1ba2

(As it is written in pure C it is not especially linked to the CPU 6502.)
The main goal of the interpreter is using it for controlling purposes in very small computers 
(embedded systems) or in synthesized environments (FPGAs).

<<<<<<< HEAD
## Implementation
### Main features

* portable source code
* 26 integer variables (A*Z) stored in separated area
* no arrays
* no strings (except for printing quoted string literals)
* integer arithmetics
* multiple statements can be written in a line separated by comma
* separated, fixed length FOR and GOSUB stacks
* tokenized storage of the program text
* calculated GOTO and GOSUB possible
* no THEN keyword, any statement can be written immediately after the conditional expression of IF
* no ELSE branch in IF statement
* if STEP keyword is omitted, FOR automatically selects 1 or -1 for STEP according to the loop limits


### BASIC statements

* LET
* PRINT
* POKE
* LIST
* NEW
* RUN
* GOTO
* GOSUB
* RETURN
* STOP
* IF
* FOR
* NEXT


### BASIC Functions

- PEEK


## Usage
### Sample BASIC program

```BASIC
10 FOR I=1TO 200
20 PRINT I;" = ";
30 N=I : GOSUB 1000
40 NEXT 
999 STOP 
1000 REM  Determine prime factors of N
1010 P=2
1020 IF (N%P)=0LET N=N/P : PRINT P;"*"; : GOTO 1020
1030 P=P+1 : IF N>1GOTO 1020
1040 PRINT N : RETURN 
```
=======
Main features:

- portable source code
- 26 integer variables (A-Z) stored in separated area
- no arrays
- no strings (except for printing quoted string literals)
- integer arithmetics
- multiple statements can be written in a line separated by comma
- separated, fixed length FOR and GOSUB stacks
- tokenized storage of the program text
- calculated GOTO and GOSUB possible
- no THEN keyword, any statement can be written immediately after the conditional expression of IF
- no ELSE branch in IF statement
- if STEP keyword is omitted, FOR automatically selects 1 or -1 for STEP according to the loop limits


Statements:

- LET
- PRINT
- POKE
- LIST
- NEW
- RUN
- GOTO
- GOSUB
- RETURN
- STOP
- IF
- FOR
- NEXT


Functions:

- PEEK
>>>>>>> f65d8784c6c7f4379ee3e7000857b306351e1ba2
