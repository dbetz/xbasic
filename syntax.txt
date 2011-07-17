Language syntax:

REM comment

INCLUDE filename-string

OPTION

DEF var = constant_expr

DEF function-name
DEF function-name ( arg [ , arg ]... )

END DEF

arg:

    var
    var variable-type
    var()
    var() variable-type
    
RETURN expr

dim-statement:

    DIM variable-def [ , variable-def ]...
    
variable-def:

    var [ variable-type ] [ section-placement ] [ scalar-initializer ]
    var ( size ) [ variable-type ] [ section-placement ] [ array-initializer ]
    
variable-type:

    AS INTEGER
    AS BYTE

section-placement:

    IN section-name-string
    
scalar-initializer:

    = constant-expr
    
array-initializer:

    = { constant-expr [ , constant-expr ]... }

[LET] var = expr

IF expr

ELSE IF expr

ELSE

END IF

SELECT expr

CASE expr [ , expr ]...

CASE expr TO expr

CASE ELSE

END SELECT

STOP

END

FOR var = start TO end [ STEP inc ]

NEXT var

DO
DO WHILE expr
DO UNTIL expr

LOOP
LOOP WHILE expr
LOOP UNTIL expr

label:

GOTO label

PRINT expr [ ;|, expr ]... [ ; ]

expr AND expr
expr OR expr

expr ^ expr
expr | expr
expr & expr

expr = expr
expr <> expr

expr < expr
expr <= expr
expr >= expr
expr > expr

expr << expr
expr >> expr

expr + expr
expr - expr
expr * expr
expr / expr
expr MOD expr

- expr
~ expr
NOT expr

function ( arg [, arg ]... )
array ( index )

(expr)
var
integer
"string"

Registers:

PAR
CNT
INA
INB
OUTA
OUTB
DIRA
DIRB
CTRA
CTRB
FRQA
FRQB
PHSA
PHSB
VCFG
VSCL
CLKFREQ
