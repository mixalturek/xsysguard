" Vim syntax file
" Language:	xsysguard
" Last Change:	2008-04-14
" Author:	Sascha Wessel <sawe@users.sf.net>
" URL:		http://xsysguard.sf.net
" License:	GPL

" ~/.vim/syntax/xsysguard.vim

if exists("b:current_syntax")
	finish
endif

syn clear
syn sync fromstart
syn sync linebreaks=1

syn keyword xsysguardTodo contained TODO FIXME XXX

syn keyword xsysguardCommand ModuleEnv SetEnv Set Line Rectangle Ellipse
syn keyword xsysguardCommand Polygon Image BarChart LineChart AreaChart Text

syn keyword xsysguardSubCommand Name Class Resource Size Position Sticky
syn keyword xsysguardSubCommand SkipTaskbar SkipPager Layer Decorations
syn keyword xsysguardSubCommand OverrideRedirect Background XShape ARGBVisual
syn keyword xsysguardSubCommand Visible Mouse Overwrite
syn keyword xsysguardSubCommand Angle ColorRange Filled Closed Min Max Mask
syn keyword xsysguardSubCommand AddPrev Background Top Alignment TabWidth

syn keyword xsysguardValue on off On Off true false True False
syn keyword xsysguardValue Above Normal Below Move Exit
syn keyword xsysguardValue CopyFromParent CopyFromRoot Color
syn keyword xsysguardValue TopLeft TopCenter TopRight CenterLeft Center
syn keyword xsysguardValue CenterRight BottomLeft BottomCenter BottomRight

syn keyword xsysguardRpnOp PI NAN INF NEGINF NOT LT LE GT GE EQ NE ISNAN ISINF
syn keyword xsysguardRpnOp ISNANZERO ISINFZERO ISNANONE ISINFONE IF
syn keyword xsysguardRpnOp MIN MAX LIMIT
syn keyword xsysguardRpnOp NEG INC DEC ADD SUB MUL DIV MOD SIN COS LOG EXP SQRT
syn keyword xsysguardRpnOp POW ATAN ATAN2 ROUND FLOOR CEIL DEG2RAD RAD2DEG ABS
syn keyword xsysguardRpnOp DUP POP EXC
syn keyword xsysguardRpnOp ATOF ATOI ATOL ATOLL STRTOF STRTOD STRTOLD
syn keyword xsysguardRpnOp STRTOL STRTOLL STRTOUL STRTOULL
syn keyword xsysguardRpnOp STRLEN STRCMP STRCASECMP STRUP STRDOWN STRREVERSE
syn keyword xsysguardRpnOp STRCHUG STRCHOMP STRTRUNCATE
syn keyword xsysguardRpnOp LOAD STORE

syn match xsysguardComment "^\s*#.*$" contains=xsysguardTodo

syn match xsysguardSpecial display contained "\\\(x\x\+\|\o\{1,3}\|.\|$\)"
syn match xsysguardSpecial display "%\(\d\+\$\)\=[-+' #0*]*\(\d*\|\*\|\*\d\+\$\)\(\.\(\d*\|\*\|\*\d\+\$\)\)\=\([hlLjzt]\|ll\|hh\)\=\([aAbdiuoxXDOUfFeEgGcCsSpn]\|\[\^\=.[^]]*\]\)" contained
syn match xsysguardSpecial display "%%" contained
syn match xsysguardSpecial display "\${[^}]*}"
syn region xsysguardString start=+L\="+ skip=+\\\\\|\\"+ end=+"+ contains=xsysguardSpecial,@Spell

syn case ignore
syn match xsysguardNumbers        display transparent "\<\d\|\.\d" contains=xsysguardNumber,xsysguardFloat,xsysguardOctal
syn match xsysguardNumbersCom     display contained transparent "\<\d\|\.\d" contains=xsysguardNumber,xsysguardFloat,xsysguardOctal
syn match xsysguardNumber         display contained "\d\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match xsysguardNumber         display contained "0x\x\+\(u\=l\{0,2}\|ll\=u\)\>"
syn match xsysguardOctal          display contained "0\o\+\(u\=l\{0,2}\|ll\=u\)\>" contains=xsysguardOctalZero
syn match xsysguardOctalZero      display contained "\<0"
syn match xsysguardFloat          display contained "\d\+f"
syn match xsysguardFloat          display contained "\d\+\.\d*\(e[-+]\=\d\+\)\=[fl]\="
syn match xsysguardFloat          display contained "\.\d\+\(e[-+]\=\d\+\)\=[fl]\=\>"
syn match xsysguardFloat          display contained "\d\+e[-+]\=\d\+[fl]\=\>"
syn match xsysguardFloat          display contained "0x\x*\.\x\+p[-+]\=\d\+[fl]\=\>"
syn match xsysguardFloat          display contained "0x\x\+\.\=p[-+]\=\d\+[fl]\=\>"
syn case match

syn match xsysguardColor "\s#\x\{3,8}"
syn match xsysguardRpnBegin "^+"
syn match xsysguardRpnDelim ","
syn match xsysguardRpnValDelim ":"

hi link xsysguardCommand	Statement
hi link xsysguardSubCommand	PreProc
hi link xsysguardValue		Type
hi link xsysguardColor		Special
hi link xsysguardRpnDelim	Operator
hi link xsysguardRpnValDelim	Identifier
hi link xsysguardRpnBegin	Statement
hi link xsysguardRpnOp		Identifier
hi link xsysguardFloat		Float
hi link xsysguardNumber		Number
hi link xsysguardTodo		Todo
hi link xsysguardString		String
hi link xsysguardComment	Comment
hi link xsysguardSpecial	Special

let b:current_syntax = "xsysguard"

